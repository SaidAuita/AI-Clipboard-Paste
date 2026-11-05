#include "BufferManager.h"
#include "ConfigManager.h"
#include "IAICopyScope.h"
#include <cstring>
#include <cmath>
#include <algorithm>

void BufferManager::SelectArt(AIArtHandle art, bool select) {
    if (!art || !sAIArt) return;
    ai::int32 attr = select ? kArtSelected : 0;
    // According to Adobe Illustrator SDK (AIArt.h), passing kArtSelected to SetArtUserAttr
    // fully selects any object including containers (groups, compound paths).
    // Sub-elements must not be manually selected to avoid corrupting compound paths and clip masks.
    sAIArt->SetArtUserAttr(art, kArtSelected, attr);
}

BufferManager& BufferManager::GetInstance() {
    static BufferManager instance;
    return instance;
}

BufferManager::BufferManager()
    : m_hasData(false),
      m_sourceDoc(nullptr),
      m_sourceArtboardIndex(0) {
    std::memset(&m_sourceBounds, 0, sizeof(m_sourceBounds));
    std::memset(&m_sourceArtboardBounds, 0, sizeof(m_sourceArtboardBounds));
}

BufferManager::~BufferManager() {
    Clear();
}

bool BufferManager::IsLayerArt(AIArtHandle art) {
    if (!art || !sAIArt) return false;
    ASBoolean isLayer = false;
    if (sAIArt->IsArtLayerGroup(art, &isLayer) == kNoErr && isLayer) {
        return true;
    }
    return false;
}

bool BufferManager::HasSelectedAncestor(AIArtHandle art, const std::vector<AIArtHandle>& allMatches) {
    if (!art || !sAIArt) return false;
    AIArtHandle curr = art;
    AIArtHandle parent = nullptr;
    while (sAIArt->GetArtParent(curr, &parent) == kNoErr && parent) {
        if (IsLayerArt(parent)) {
            // Достигли уровня слоя: выше предков-групп нет
            break;
        }
        for (AIArtHandle m : allMatches) {
            if (m == parent) {
                return true; // Предок уже есть в списке выделенных
            }
        }
        curr = parent;
    }
    return false;
}

bool BufferManager::IsSourceDocOpen() const {
    if (!m_sourceDoc) return false;
    if (!sAIDocumentList) {
        AIDocumentHandle cur = nullptr;
        if (sAIDocument && sAIDocument->GetDocument(&cur) == kNoErr) {
            return (cur == m_sourceDoc);
        }
        return false;
    }
    ai::int32 docCount = 0;
    if (sAIDocumentList->Count(&docCount) == kNoErr) {
        for (ai::int32 i = 0; i < docCount; ++i) {
            AIDocumentHandle d = nullptr;
            if (sAIDocumentList->GetNthDocument(&d, i) == kNoErr && d == m_sourceDoc) {
                return true;
            }
        }
    }
    return false;
}

void BufferManager::Clear() {
    if (sAIArt && !m_clonedItems.empty() && m_sourceDoc) {
        if (IsSourceDocOpen()) {
            for (AIArtHandle item : m_clonedItems) {
                if (item) {
                    sAIArt->DisposeArt(item);
                }
            }
        }
    }
    m_clonedItems.clear();
    m_hasData = false;
    m_sourceDoc = nullptr;
    std::memset(&m_sourceBounds, 0, sizeof(m_sourceBounds));
    std::memset(&m_sourceArtboardBounds, 0, sizeof(m_sourceArtboardBounds));
    m_sourceArtboardIndex = 0;
}

void BufferManager::CalculateTotalBounds(const std::vector<AIArtHandle>& items, AIRealRect& outRect) {
    if (items.empty() || !sAIArt) return;

    outRect.left = 1e9;
    outRect.top = -1e9;
    outRect.right = -1e9;
    outRect.bottom = 1e9;

    for (AIArtHandle item : items) {
        AIRealRect itemBounds;
        if (sAIArt->GetArtBounds(item, &itemBounds) == kNoErr) {
            outRect.left = (std::min)(outRect.left, itemBounds.left);
            outRect.top = (std::max)(outRect.top, itemBounds.top);
            outRect.right = (std::max)(outRect.right, itemBounds.right);
            outRect.bottom = (std::min)(outRect.bottom, itemBounds.bottom);
        }
    }
}

void BufferManager::TransformArtRecursive(AIArtHandle art, const AIRealMatrix& matrix) {
    if (!sAIArt || !sAITransformArt || !art) return;

    short type = 0;
    if (sAIArt->GetArtType(art, &type) != kNoErr) return;

    // Groups and compound paths require recursive traversal to transform member artwork
    if (type == kGroupArt || type == kCompoundPathArt) {
        AIArtHandle child = nullptr;
        if (sAIArt->GetArtFirstChild(art, &child) == kNoErr) {
            while (child) {
                TransformArtRecursive(child, matrix);
                AIArtHandle next = nullptr;
                if (sAIArt->GetArtSibling(child, &next) == kNoErr) {
                    child = next;
                } else {
                    break;
                }
            }
        }
        return;
    }

    ai::int32 flags = kTransformObjects | kTransformChildren | kTransformEntireArtStyle;
    AIErr transErr = sAITransformArt->TransformArt(art, const_cast<AIRealMatrix*>(&matrix), 1.0, flags);
    if (transErr != kNoErr) {
        PluginLog("  [TransformArtRecursive WARN] TransformArt err=%d for art %p (type=%d)", transErr, art, type);
    }
}

void BufferManager::OffsetArt(AIArtHandle art, double dx, double dy) {
    if (std::abs(dx) < 1e-6 && std::abs(dy) < 1e-6) return;
    if (!art) return;

    AIRealMatrix matrix;
    matrix.a = 1.0;
    matrix.b = 0.0;
    matrix.c = 0.0;
    matrix.d = 1.0;
    matrix.tx = dx;
    matrix.ty = dy;

    TransformArtRecursive(art, matrix);
}

bool BufferManager::CopySelection() {
    if (!sAIMatchingArt || !sAIArt || !sAIDocument || !sAIArtboard) {
        PluginLog("[CopySelection FAIL] Suites not acquired!");
        return false;
    }

    AIDocumentHandle doc = nullptr;
    AIErr docErr = sAIDocument->GetDocument(&doc);
    if (docErr != kNoErr || !doc) {
        PluginLog("[CopySelection FAIL] No active document (err=%d, doc=%p)", docErr, doc);
        return false;
    }

    AIArtHandle** matches = nullptr;
    ai::int32 count = 0;
    AIErr err = sAIMatchingArt->GetSelectedArt(&matches, &count);
    PluginLog("[CopySelection] sAIMatchingArt->GetSelectedArt: err=%d, count=%d, matches=%p", err, count, matches);

    if (err != kNoErr || count <= 0 || !matches || !(*matches)) {
        PluginLog("[CopySelection] Nothing selected");
        return false;
    }

    std::vector<AIArtHandle> rawList;
    rawList.reserve(count);
    for (ai::int32 i = 0; i < count; ++i) {
        AIArtHandle h = (*matches)[i];
        if (h) rawList.push_back(h);
    }

    if (sAIMdMemory) {
        sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
        matches = nullptr;
    }

    // 1. Фильтруем: исключаем слои и исключаем дочерние элементы выделенных групп
    std::vector<AIArtHandle> topArts;
    for (AIArtHandle h : rawList) {
        if (IsLayerArt(h)) {
            PluginLog("  [CopySelection] Skipped Layer Group item %p", h);
            continue;
        }
        if (HasSelectedAncestor(h, rawList)) {
            PluginLog("  [CopySelection] Skipped child item %p (parent group is already selected)", h);
            continue;
        }
        topArts.push_back(h);
    }

    if (topArts.empty()) {
        PluginLog("[CopySelection FAIL] No graphic art items found in selection (only layers or invalid)");
        return false;
    }

    // 2. Очищаем предыдущий буфер
    Clear();
    m_sourceDoc = doc;

    // 3. Вычисляем границы исходных объектов
    CalculateTotalBounds(topArts, m_sourceBounds);

    // 4. Запоминаем текущий артборд
    ai::ArtboardList artboardList;
    if (sAIArtboard->GetArtboardList(artboardList) == kNoErr) {
        sAIArtboard->GetActive(artboardList, m_sourceArtboardIndex);
        ai::ArtboardProperties props;
        sAIArtboard->Init(props);
        if (sAIArtboard->GetArtboardProperties(artboardList, m_sourceArtboardIndex, props) == kNoErr) {
            sAIArtboard->GetPosition(props, m_sourceArtboardBounds);
        }
        sAIArtboard->Dispose(props);

        // Проверяем, действительно ли центр выделенных объектов лежит в границах активного артборда.
        // Если пользователь выделил объект на другом артборде без смены активного — ищем фактический артборд.
        double scx = (m_sourceBounds.left + m_sourceBounds.right) / 2.0;
        double scy = (m_sourceBounds.top + m_sourceBounds.bottom) / 2.0;
        bool insideActive = (scx >= m_sourceArtboardBounds.left && scx <= m_sourceArtboardBounds.right &&
                             scy <= m_sourceArtboardBounds.top && scy >= m_sourceArtboardBounds.bottom);
        if (!insideActive) {
            ai::ArtboardID abCount = 0;
            if (sAIArtboard->GetCount(artboardList, abCount) == kNoErr && abCount > 1) {
                for (ai::ArtboardID i = 0; i < abCount; ++i) {
                    if (i == m_sourceArtboardIndex) continue;
                    ai::ArtboardProperties p;
                    sAIArtboard->Init(p);
                    if (sAIArtboard->GetArtboardProperties(artboardList, i, p) == kNoErr) {
                        AIRealRect b;
                        sAIArtboard->GetPosition(p, b);
                        if (scx >= b.left && scx <= b.right && scy <= b.top && scy >= b.bottom) {
                            m_sourceArtboardIndex = i;
                            m_sourceArtboardBounds = b;
                            sAIArtboard->Dispose(p);
                            PluginLog("  [CopySelection] Detected true source artboard #%d containing selection", i);
                            break;
                        }
                    }
                    sAIArtboard->Dispose(p);
                }
            }
        }
        sAIArtboard->ReleaseArtboardList(artboardList);
    }

    // 5. Получаем текущий активный слой документа
    AIArtHandle currentLayerGroup = nullptr;
    sAIArt->GetFirstArtOfLayer(nullptr, &currentLayerGroup);

    // 6. Создаем глубокие скрытые копии-снапшоты в буфер
    ai::CopyScope copyScope;
    copyScope.Initialize(kAICopyScopeForDuplicate, std::nothrow);

    AIArtHandle prevSnapshot = nullptr;
    for (AIArtHandle src : topArts) {
        if (!sAIArt->ValidArt(src, true)) continue;

        AIArtHandle snapshot = nullptr;
        AIErr dupErr = kNoErr;
        if (!prevSnapshot) {
            if (currentLayerGroup) {
                dupErr = sAIArt->DuplicateArt(src, kPlaceInsideOnBottom, currentLayerGroup, &snapshot);
            } else {
                dupErr = sAIArt->DuplicateArt(src, kPlaceAbove, src, &snapshot);
            }
        } else {
            dupErr = sAIArt->DuplicateArt(src, kPlaceBelow, prevSnapshot, &snapshot);
        }

        if (dupErr == kNoErr && snapshot) {
            prevSnapshot = snapshot;
            // Скрываем snapshot от пользователя и снимаем выделение
            sAIArt->SetArtUserAttr(snapshot, kArtHidden, kArtHidden);
            sAIArt->SetArtUserAttr(snapshot, kArtSelected, 0);
            m_clonedItems.push_back(snapshot);
            PluginLog("  [CopySelection] Created hidden buffer snapshot %p for source %p", snapshot, src);
        } else {
            PluginLog("  [CopySelection ERROR] DuplicateArt failed (err=%d) for source %p", dupErr, src);
        }
    }

    if (m_clonedItems.empty()) {
        PluginLog("[CopySelection FAIL] Failed to create buffer snapshots");
        return false;
    }

    m_hasData = true;
    PluginLog("[CopySelection SUCCESS] Stored %zu top-level graphic item(s) in hidden buffer", m_clonedItems.size());
    return true;
}

bool BufferManager::PasteInPlace() {
    if (!m_hasData || m_clonedItems.empty() || !sAIArt || !sAIDocument || !sAIArtboard) {
        PluginLog("[PasteInPlace FAIL] Buffer is empty");
        return false;
    }

    AIDocumentHandle targetDoc = nullptr;
    AIErr targetErr = sAIDocument->GetDocument(&targetDoc);
    if (targetErr != kNoErr || !targetDoc) {
        PluginLog("[PasteInPlace FAIL] Target document error (err=%d, doc=%p)", targetErr, targetDoc);
        return false;
    }

    if (!IsSourceDocOpen()) {
        PluginLog("[PasteInPlace FAIL] Source document has been closed! Clearing buffer");
        Clear();
        return false;
    }

    bool isSameDoc = (targetDoc == m_sourceDoc);
    PluginLog("[PasteInPlace] targetDoc=%p, sourceDoc=%p (isSameDoc=%d)", targetDoc, m_sourceDoc, isSameDoc);

    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Paste In Place (Buffer)"), ai::UnicodeString("Undo Paste In Place"));
    }

    // Определяем целевой артборд
    ai::ArtboardID targetArtboardIdx = 0;
    AIRealRect targetArtboardBounds = {0, 0, 0, 0};
    ai::ArtboardList artboardList;
    if (sAIArtboard->GetArtboardList(artboardList) == kNoErr) {
        sAIArtboard->GetActive(artboardList, targetArtboardIdx);
        ai::ArtboardProperties props;
        sAIArtboard->Init(props);
        if (sAIArtboard->GetArtboardProperties(artboardList, targetArtboardIdx, props) == kNoErr) {
            sAIArtboard->GetPosition(props, targetArtboardBounds);
        }
        sAIArtboard->Dispose(props);
        sAIArtboard->ReleaseArtboardList(artboardList);
    }

    // Если целевой артборд или документ отличается от исходного — смещаем относительно координат артборда
    // Если артборд ТОТ ЖЕ САМЫЙ в том же документе — dx=0, dy=0 (вставка точно на исходные координаты)
    double dx = 0.0;
    double dy = 0.0;
    if (!isSameDoc || targetArtboardIdx != m_sourceArtboardIndex) {
        dx = targetArtboardBounds.left - m_sourceArtboardBounds.left;
        dy = targetArtboardBounds.top - m_sourceArtboardBounds.top;
    }

    // Получаем группу ТЕКУЩЕГО активного слоя
    AIArtHandle targetLayerGroup = nullptr;
    sAIArt->GetFirstArtOfLayer(nullptr, &targetLayerGroup);

    // Снимаем выделение со всех текущих объектов целевого документа
    if (sAIMatchingArt) {
        sAIMatchingArt->DeselectAll();
    }

    ai::CopyScope copyScope;
    AICopyScopeKind scopeKind = isSameDoc ? kAICopyScopeForDuplicate : kAICopyScopeForRetarget;
    copyScope.Initialize(scopeKind, std::nothrow);

    AIArtHandle prevArt = nullptr;
    std::vector<AIArtHandle> newItems;
    for (AIArtHandle snapshot : m_clonedItems) {
        if (!snapshot) continue;
        if (isSameDoc && !sAIArt->ValidArt(snapshot, true)) {
            PluginLog("  [PasteInPlace] Snapshot %p is invalid in layer tree, skipping", snapshot);
            continue;
        }

        AIArtHandle newArt = nullptr;
        AIErr cloneErr = kNoErr;
        if (!prevArt) {
            if (targetLayerGroup) {
                // Первый (самый верхний) объект помещаем наверх слоя
                cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceInsideOnTop, targetLayerGroup, &newArt);
            } else {
                cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceAbove, snapshot, &newArt);
            }
        } else {
            // Последующие объекты вставляем строго ПОД предыдущим, сохраняя исходный Z-порядок!
            cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceBelow, prevArt, &newArt);
        }

        if (cloneErr == kNoErr && newArt) {
            prevArt = newArt;
            // Делаем видимым
            sAIArt->SetArtUserAttr(newArt, kArtHidden, 0);

            if (std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6) {
                OffsetArt(newArt, dx, dy);
            }
            newItems.push_back(newArt);
            PluginLog("  [PasteInPlace] Successfully pasted item %p into active layer (isSameDoc=%d)", newArt, isSameDoc);
        } else {
            PluginLog("  [PasteInPlace FAIL] DuplicateArt error %d for snapshot %p (isSameDoc=%d)", cloneErr, snapshot, isSameDoc);
        }
    }

    // Завершаем транзакцию CopyScope до выделения и перерисовки
    copyScope.Terminate(std::nothrow);

    // Применяем выделение к вставленным объектам (согласно настройкам)
    bool selectAfter = ConfigManager::GetInstance().GetSelectAfterPaste();
    if (selectAfter) {
        for (AIArtHandle item : newItems) {
            SelectArt(item, true);
        }
    }

    sAIDocument->RedrawDocument();
    PluginLog("[PasteInPlace SUCCESS] Duplicated %zu item(s) (dx=%.2f, dy=%.2f, isSameDoc=%d)",
              newItems.size(), dx, dy, isSameDoc);
    return !newItems.empty();
}

bool BufferManager::PasteCenter() {
    if (!m_hasData || m_clonedItems.empty() || !sAIArt || !sAIDocument) {
        PluginLog("[PasteCenter FAIL] Buffer is empty");
        return false;
    }

    AIDocumentHandle targetDoc = nullptr;
    AIErr targetErr = sAIDocument->GetDocument(&targetDoc);
    if (targetErr != kNoErr || !targetDoc) {
        PluginLog("[PasteCenter FAIL] Target document error (err=%d, doc=%p)", targetErr, targetDoc);
        return false;
    }

    if (!IsSourceDocOpen()) {
        PluginLog("[PasteCenter FAIL] Source document has been closed! Clearing buffer");
        Clear();
        return false;
    }

    bool isSameDoc = (targetDoc == m_sourceDoc);
    PluginLog("[PasteCenter] targetDoc=%p, sourceDoc=%p (isSameDoc=%d)", targetDoc, m_sourceDoc, isSameDoc);

    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Paste to Center (Buffer)"), ai::UnicodeString("Undo Paste to Center"));
    }

    // 1. Получаем группу ТЕКУЩЕГО активного слоя
    AIArtHandle targetLayerGroup = nullptr;
    sAIArt->GetFirstArtOfLayer(nullptr, &targetLayerGroup);

    // 2. Снимаем выделение со всех существующих объектов
    if (sAIMatchingArt) {
        sAIMatchingArt->DeselectAll();
    }

    ai::CopyScope copyScope;
    AICopyScopeKind scopeKind = isSameDoc ? kAICopyScopeForDuplicate : kAICopyScopeForRetarget;
    copyScope.Initialize(scopeKind, std::nothrow);

    // 3. Дублируем все снапшоты в целевой документ на текущий слой, сохраняя точный Z-порядок
    AIArtHandle prevArt = nullptr;
    std::vector<AIArtHandle> newItems;
    for (AIArtHandle snapshot : m_clonedItems) {
        if (!snapshot) continue;
        if (isSameDoc && !sAIArt->ValidArt(snapshot, true)) {
            PluginLog("  [PasteCenter] Snapshot %p is invalid in layer tree, skipping", snapshot);
            continue;
        }

        AIArtHandle newArt = nullptr;
        AIErr cloneErr = kNoErr;
        if (!prevArt) {
            if (targetLayerGroup) {
                // Первый (самый верхний) объект помещаем наверх слоя
                cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceInsideOnTop, targetLayerGroup, &newArt);
            } else {
                cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceAbove, snapshot, &newArt);
            }
        } else {
            // Последующие объекты вставляем строго ПОД предыдущим, сохраняя исходный Z-порядок!
            cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceBelow, prevArt, &newArt);
        }

        if (cloneErr == kNoErr && newArt) {
            prevArt = newArt;
            sAIArt->SetArtUserAttr(newArt, kArtHidden, 0);
            newItems.push_back(newArt);
            PluginLog("  [PasteCenter] Duplicated item %p into target layer (isSameDoc=%d)", newArt, isSameDoc);
        } else {
            PluginLog("  [PasteCenter FAIL] DuplicateArt error %d for snapshot %p (isSameDoc=%d)", cloneErr, snapshot, isSameDoc);
        }
    }

    if (newItems.empty()) {
        copyScope.Terminate(std::nothrow);
        PluginLog("[PasteCenter FAIL] No items could be duplicated");
        return false;
    }

    // Завершаем транзакцию CopyScope до смещения, выделения и перерисовки
    copyScope.Terminate(std::nothrow);

    // 4. Определяем актуальный центр области просмотра (Viewport Center) целевого документа
    AIRealPoint viewCenter = {0.0, 0.0};
    bool gotCenter = false;
    if (sAIDocumentView) {
        AIRealRect viewBounds = {0};
        AIErr bErr = sAIDocumentView->GetDocumentViewBounds(nullptr, &viewBounds);
        AIErr cErr = sAIDocumentView->GetDocumentViewCenter(nullptr, &viewCenter);
        if (cErr == kNoErr) {
            gotCenter = true;
            PluginLog("  [PasteCenter] Viewport Center (via GetDocumentViewCenter): (%.2f, %.2f)", viewCenter.h, viewCenter.v);
        } else if (bErr == kNoErr) {
            viewCenter.h = (viewBounds.left + viewBounds.right) / 2.0;
            viewCenter.v = (viewBounds.top + viewBounds.bottom) / 2.0;
            gotCenter = true;
            PluginLog("  [PasteCenter] Viewport Center (via GetDocumentViewBounds): (%.2f, %.2f)", viewCenter.h, viewCenter.v);
        }
    }

    if (!gotCenter && sAIArtboard) {
        // Fallback: центр активного артборда целевого документа
        ai::ArtboardID targetArtboardIdx = 0;
        AIRealRect targetArtboardBounds = {0, 0, 0, 0};
        ai::ArtboardList artboardList;
        if (sAIArtboard->GetArtboardList(artboardList) == kNoErr) {
            sAIArtboard->GetActive(artboardList, targetArtboardIdx);
            ai::ArtboardProperties props;
            sAIArtboard->Init(props);
            if (sAIArtboard->GetArtboardProperties(artboardList, targetArtboardIdx, props) == kNoErr) {
                sAIArtboard->GetPosition(props, targetArtboardBounds);
            }
            sAIArtboard->Dispose(props);
            sAIArtboard->ReleaseArtboardList(artboardList);
        }
        viewCenter.h = (targetArtboardBounds.left + targetArtboardBounds.right) / 2.0;
        viewCenter.v = (targetArtboardBounds.top + targetArtboardBounds.bottom) / 2.0;
        gotCenter = true;
        PluginLog("  [PasteCenter] Fallback Artboard Center: (%.2f, %.2f)", viewCenter.h, viewCenter.v);
    }

    // 5. Вычисляем ФАКТИЧЕСКИЙ совокупный центр всех вставленных объектов в целевом документе
    AIRealRect targetBounds = {0};
    CalculateTotalBounds(newItems, targetBounds);
    double targetCenterX = (targetBounds.left + targetBounds.right) / 2.0;
    double targetCenterY = (targetBounds.top + targetBounds.bottom) / 2.0;

    double dx = viewCenter.h - targetCenterX;
    double dy = viewCenter.v - targetCenterY;
    PluginLog("  [PasteCenter] Target Items Center: (%.2f, %.2f), Target View Center: (%.2f, %.2f) -> dx=%.2f, dy=%.2f",
              targetCenterX, targetCenterY, viewCenter.h, viewCenter.v, dx, dy);

    // 6. Смещаем все созданные объекты точно в центр видимой области экрана
    if (std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6) {
        for (AIArtHandle newArt : newItems) {
            OffsetArt(newArt, dx, dy);
        }
    }

    // 7. Применяем выделение к вставленным объектам (согласно настройкам)
    bool selectAfter = ConfigManager::GetInstance().GetSelectAfterPaste();
    if (selectAfter) {
        for (AIArtHandle item : newItems) {
            SelectArt(item, true);
        }
    }

    sAIDocument->RedrawDocument();
    PluginLog("[PasteCenter SUCCESS] Duplicated %zu item(s) to view center (dx=%.2f, dy=%.2f, isSameDoc=%d)",
              newItems.size(), dx, dy, isSameDoc);
    return true;
}

bool BufferManager::PasteAllArtboards() {
    if (!m_hasData || m_clonedItems.empty() || !sAIArt || !sAIDocument || !sAIArtboard) {
        PluginLog("[PasteAllArtboards FAIL] Buffer is empty or suites missing");
        return false;
    }

    AIDocumentHandle targetDoc = nullptr;
    AIErr targetErr = sAIDocument->GetDocument(&targetDoc);
    if (targetErr != kNoErr || !targetDoc) {
        PluginLog("[PasteAllArtboards FAIL] Target document error (err=%d, doc=%p)", targetErr, targetDoc);
        return false;
    }

    if (!IsSourceDocOpen()) {
        PluginLog("[PasteAllArtboards FAIL] Source document has been closed! Clearing buffer");
        Clear();
        return false;
    }

    bool isSameDoc = (targetDoc == m_sourceDoc);
    PluginLog("[PasteAllArtboards] targetDoc=%p, sourceDoc=%p (isSameDoc=%d)", targetDoc, m_sourceDoc, isSameDoc);

    ai::ArtboardList artboardList;
    if (sAIArtboard->GetArtboardList(artboardList) != kNoErr) {
        PluginLog("[PasteAllArtboards FAIL] GetArtboardList failed");
        return false;
    }

    ai::ArtboardID abCount = 0;
    sAIArtboard->GetCount(artboardList, abCount);
    if (abCount <= 0) {
        sAIArtboard->ReleaseArtboardList(artboardList);
        PluginLog("[PasteAllArtboards FAIL] No artboards found in target document");
        return false;
    }

    struct ArtboardInfo {
        ai::ArtboardID id;
        AIRealRect bounds;
    };
    std::vector<ArtboardInfo> artboards;
    artboards.reserve(abCount);

    for (ai::ArtboardID i = 0; i < abCount; ++i) {
        ai::ArtboardProperties props;
        sAIArtboard->Init(props);
        if (sAIArtboard->GetArtboardProperties(artboardList, i, props) == kNoErr) {
            AIRealRect b = {0};
            sAIArtboard->GetPosition(props, b);
            artboards.push_back({ i, b });
        }
        sAIArtboard->Dispose(props);
    }
    sAIArtboard->ReleaseArtboardList(artboardList);

    if (artboards.empty()) {
        PluginLog("[PasteAllArtboards FAIL] Artboard list is empty");
        return false;
    }

    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Paste on All Artboards (Buffer)"), ai::UnicodeString("Undo Paste on All Artboards"));
    }

    // Получаем группу ТЕКУЩЕГО активного слоя
    AIArtHandle targetLayerGroup = nullptr;
    sAIArt->GetFirstArtOfLayer(nullptr, &targetLayerGroup);

    // Снимаем выделение со всех текущих объектов целевого документа
    if (sAIMatchingArt) {
        sAIMatchingArt->DeselectAll();
    }

    ai::CopyScope copyScope;
    AICopyScopeKind scopeKind = isSameDoc ? kAICopyScopeForDuplicate : kAICopyScopeForRetarget;
    copyScope.Initialize(scopeKind, std::nothrow);

    AIArtHandle prevArt = nullptr;
    std::vector<AIArtHandle> allNewItems;

    // Вставляем копии на каждый артборд в то же самое место
    for (const auto& ab : artboards) {
        double dx = ab.bounds.left - m_sourceArtboardBounds.left;
        double dy = ab.bounds.top - m_sourceArtboardBounds.top;

        for (AIArtHandle snapshot : m_clonedItems) {
            if (!snapshot) continue;
            if (isSameDoc && !sAIArt->ValidArt(snapshot, true)) {
                PluginLog("  [PasteAllArtboards] Snapshot %p is invalid in layer tree, skipping", snapshot);
                continue;
            }

            AIArtHandle newArt = nullptr;
            AIErr cloneErr = kNoErr;
            if (!prevArt) {
                if (targetLayerGroup) {
                    cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceInsideOnTop, targetLayerGroup, &newArt);
                } else {
                    cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceAbove, snapshot, &newArt);
                }
            } else {
                cloneErr = sAIArt->DuplicateArt(snapshot, kPlaceBelow, prevArt, &newArt);
            }

            if (cloneErr == kNoErr && newArt) {
                prevArt = newArt;
                sAIArt->SetArtUserAttr(newArt, kArtHidden, 0);

                if (std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6) {
                    OffsetArt(newArt, dx, dy);
                }
                allNewItems.push_back(newArt);
            } else {
                PluginLog("  [PasteAllArtboards FAIL] DuplicateArt error %d for snapshot %p (abId=%d)", cloneErr, snapshot, ab.id);
            }
        }
    }

    // Завершаем транзакцию CopyScope до выделения и перерисовки
    copyScope.Terminate(std::nothrow);

    // Применяем выделение к вставленным объектам (согласно настройкам)
    bool selectAfter = ConfigManager::GetInstance().GetSelectAfterPaste();
    if (selectAfter) {
        for (AIArtHandle item : allNewItems) {
            SelectArt(item, true);
        }
    }

    sAIDocument->RedrawDocument();
    PluginLog("[PasteAllArtboards SUCCESS] Duplicated %zu item(s) across %zu artboard(s) (isSameDoc=%d)",
              allNewItems.size(), artboards.size(), isSameDoc);
    return !allNewItems.empty();
}

bool BufferManager::GetSelectionDimensions(double& outW, double& outH) {
    outW = 0.0;
    outH = 0.0;
    if (!sAIMatchingArt || !sAIArt || !sAIDocument) return false;

    AIDocumentHandle doc = nullptr;
    if (sAIDocument->GetDocument(&doc) != kNoErr || !doc) return false;

    AIArtHandle** matches = nullptr;
    ai::int32 count = 0;
    if (sAIMatchingArt->GetSelectedArt(&matches, &count) != kNoErr || count <= 0 || !matches || !(*matches)) {
        return false;
    }

    std::vector<AIArtHandle> rawList;
    rawList.reserve(count);
    for (ai::int32 i = 0; i < count; ++i) {
        AIArtHandle h = (*matches)[i];
        if (h) rawList.push_back(h);
    }
    if (sAIMdMemory) {
        sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
    }

    std::vector<AIArtHandle> topArts;
    for (AIArtHandle h : rawList) {
        if (!IsLayerArt(h) && !HasSelectedAncestor(h, rawList)) {
            topArts.push_back(h);
        }
    }
    if (topArts.empty()) return false;

    AIRealRect bounds = {0};
    CalculateTotalBounds(topArts, bounds);
    outW = std::abs(bounds.right - bounds.left);
    outH = std::abs(bounds.top - bounds.bottom);
    return true;
}

bool BufferManager::CloneSelectionWithOffset(int direction, double gap) {
    if (!sAIMatchingArt || !sAIArt || !sAIDocument || !sAITransformArt) {
        PluginLog("[CloneSelectionWithOffset FAIL] Suites not acquired!");
        return false;
    }

    AIDocumentHandle doc = nullptr;
    if (sAIDocument->GetDocument(&doc) != kNoErr || !doc) {
        PluginLog("[CloneSelectionWithOffset FAIL] No active document");
        return false;
    }

    AIArtHandle** matches = nullptr;
    ai::int32 count = 0;
    AIErr err = sAIMatchingArt->GetSelectedArt(&matches, &count);
    if (err != kNoErr || count <= 0 || !matches || !(*matches)) {
        PluginLog("[CloneSelectionWithOffset] Nothing selected");
        return false;
    }

    std::vector<AIArtHandle> rawList;
    rawList.reserve(count);
    for (ai::int32 i = 0; i < count; ++i) {
        AIArtHandle h = (*matches)[i];
        if (h) rawList.push_back(h);
    }
    if (sAIMdMemory) {
        sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
    }

    std::vector<AIArtHandle> topArts;
    for (AIArtHandle h : rawList) {
        if (!IsLayerArt(h) && !HasSelectedAncestor(h, rawList)) {
            topArts.push_back(h);
        }
    }
    if (topArts.empty()) {
        PluginLog("[CloneSelectionWithOffset FAIL] No valid graphic art items found in selection");
        return false;
    }

    AIRealRect bounds = {0};
    CalculateTotalBounds(topArts, bounds);
    double W = std::abs(bounds.right - bounds.left);
    double H = std::abs(bounds.top - bounds.bottom);
    if (W < 1e-6 && H < 1e-6) {
        PluginLog("[CloneSelectionWithOffset FAIL] Selection dimensions are zero");
        return false;
    }

    double stepX = W + gap;
    double stepY = H + gap;
    double dx = 0.0;
    double dy = 0.0;

    switch (direction) {
        case 8: // Up (Numpad 8)
            dx = 0.0; dy = stepY; break;
        case 2: // Down (Numpad 2)
            dx = 0.0; dy = -stepY; break;
        case 4: // Left (Numpad 4)
            dx = -stepX; dy = 0.0; break;
        case 6: // Right (Numpad 6)
            dx = stepX; dy = 0.0; break;
        case 7: // Up-Left (Numpad 7)
            dx = -stepX; dy = stepY; break;
        case 9: // Up-Right (Numpad 9)
            dx = stepX; dy = stepY; break;
        case 1: // Down-Left (Numpad 1)
            dx = -stepX; dy = -stepY; break;
        case 3: // Down-Right (Numpad 3)
            dx = stepX; dy = -stepY; break;
        case 5: // Center / In-Place clone
            dx = 0.0; dy = 0.0; break;
        default:
            PluginLog("[CloneSelectionWithOffset FAIL] Invalid direction %d", direction);
            return false;
    }

    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Pattern Offset Clone"), ai::UnicodeString("Undo Pattern Offset Clone"));
    }

    ai::CopyScope copyScope;
    copyScope.Initialize(kAICopyScopeForDuplicate, std::nothrow);

    std::vector<AIArtHandle> newItems;
    newItems.reserve(topArts.size());

    for (AIArtHandle src : topArts) {
        if (!src) continue;
        AIArtHandle newArt = nullptr;
        AIErr cloneErr = sAIArt->DuplicateArt(src, kPlaceAbove, src, &newArt);
        if (cloneErr == kNoErr && newArt) {
            sAIArt->SetArtUserAttr(newArt, kArtHidden, 0);
            if (std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6) {
                OffsetArt(newArt, dx, dy);
            }
            newItems.push_back(newArt);
        } else {
            PluginLog("  [CloneSelectionWithOffset ERROR] DuplicateArt err=%d for %p", cloneErr, src);
        }
    }

    copyScope.Terminate(std::nothrow);

    if (!newItems.empty()) {
        sAIMatchingArt->DeselectAll();
        for (AIArtHandle item : newItems) {
            SelectArt(item, true);
        }
    }

    sAIDocument->RedrawDocument();
    PluginLog("[CloneSelectionWithOffset SUCCESS] Cloned %zu item(s) dir=%d (dx=%.2f, dy=%.2f, W=%.2f, H=%.2f)",
              newItems.size(), direction, dx, dy, W, H);
    return !newItems.empty();
}

bool BufferManager::MoveSelectionWithOffset(int direction, double gap) {
    if (!sAIMatchingArt || !sAIArt || !sAIDocument || !sAITransformArt) {
        PluginLog("[MoveSelectionWithOffset FAIL] Suites not acquired!");
        return false;
    }

    AIDocumentHandle doc = nullptr;
    if (sAIDocument->GetDocument(&doc) != kNoErr || !doc) {
        PluginLog("[MoveSelectionWithOffset FAIL] No active document");
        return false;
    }

    AIArtHandle** matches = nullptr;
    ai::int32 count = 0;
    AIErr err = sAIMatchingArt->GetSelectedArt(&matches, &count);
    if (err != kNoErr || count <= 0 || !matches || !(*matches)) {
        PluginLog("[MoveSelectionWithOffset] Nothing selected");
        return false;
    }

    std::vector<AIArtHandle> rawList;
    rawList.reserve(count);
    for (ai::int32 i = 0; i < count; ++i) {
        AIArtHandle h = (*matches)[i];
        if (h) rawList.push_back(h);
    }
    if (sAIMdMemory) {
        sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
    }

    std::vector<AIArtHandle> topArts;
    for (AIArtHandle h : rawList) {
        if (!IsLayerArt(h) && !HasSelectedAncestor(h, rawList)) {
            topArts.push_back(h);
        }
    }
    if (topArts.empty()) {
        PluginLog("[MoveSelectionWithOffset FAIL] No valid graphic art items found in selection");
        return false;
    }

    AIRealRect bounds = {0};
    CalculateTotalBounds(topArts, bounds);
    double W = std::abs(bounds.right - bounds.left);
    double H = std::abs(bounds.top - bounds.bottom);
    if (W < 1e-6 && H < 1e-6) {
        PluginLog("[MoveSelectionWithOffset FAIL] Selection dimensions are zero");
        return false;
    }

    double stepX = W + gap;
    double stepY = H + gap;
    double dx = 0.0;
    double dy = 0.0;

    switch (direction) {
        case 8: // Up (Numpad 8)
            dx = 0.0; dy = stepY; break;
        case 2: // Down (Numpad 2)
            dx = 0.0; dy = -stepY; break;
        case 4: // Left (Numpad 4)
            dx = -stepX; dy = 0.0; break;
        case 6: // Right (Numpad 6)
            dx = stepX; dy = 0.0; break;
        case 7: // Up-Left (Numpad 7)
            dx = -stepX; dy = stepY; break;
        case 9: // Up-Right (Numpad 9)
            dx = stepX; dy = stepY; break;
        case 1: // Down-Left (Numpad 1)
            dx = -stepX; dy = -stepY; break;
        case 3: // Down-Right (Numpad 3)
            dx = stepX; dy = -stepY; break;
        case 5: // Center / In-Place (no shift)
            dx = 0.0; dy = 0.0; break;
        default:
            PluginLog("[MoveSelectionWithOffset FAIL] Invalid direction %d", direction);
            return false;
    }

    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Pattern Offset Move"), ai::UnicodeString("Undo Pattern Offset Move"));
    }

    for (AIArtHandle src : topArts) {
        if (!src) continue;
        if (std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6) {
            OffsetArt(src, dx, dy);
        }
    }

    sAIDocument->RedrawDocument();
    PluginLog("[MoveSelectionWithOffset SUCCESS] Moved %zu item(s) dir=%d (dx=%.2f, dy=%.2f, W=%.2f, H=%.2f)",
              topArts.size(), direction, dx, dy, W, H);
    return true;
}

bool BufferManager::ReflectSelectionVertical() {
    return ReflectSelection(false);
}

bool BufferManager::ReflectSelectionHorizontal() {
    return ReflectSelection(true);
}

bool BufferManager::ReflectSelection(bool horizontalAxis) {
    if (!sAIMatchingArt || !sAIArt || !sAIDocument || !sAITransformArt) {
        PluginLog("[ReflectSelection FAIL] Suites not acquired!");
        return false;
    }

    AIDocumentHandle doc = nullptr;
    if (sAIDocument->GetDocument(&doc) != kNoErr || !doc) {
        PluginLog("[ReflectSelection FAIL] No active document");
        return false;
    }

    AIArtHandle** matches = nullptr;
    ai::int32 count = 0;
    AIErr err = sAIMatchingArt->GetSelectedArt(&matches, &count);
    if (err != kNoErr || count <= 0 || !matches || !(*matches)) {
        PluginLog("[ReflectSelection] Nothing selected");
        return false;
    }

    std::vector<AIArtHandle> rawList;
    rawList.reserve(count);
    for (ai::int32 i = 0; i < count; ++i) {
        AIArtHandle h = (*matches)[i];
        if (h) rawList.push_back(h);
    }
    if (sAIMdMemory) {
        sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
    }

    std::vector<AIArtHandle> topArts;
    for (AIArtHandle h : rawList) {
        if (!IsLayerArt(h) && !HasSelectedAncestor(h, rawList)) {
            topArts.push_back(h);
        }
    }
    if (topArts.empty()) {
        PluginLog("[ReflectSelection FAIL] No valid graphic art items found in selection");
        return false;
    }

    AIRealRect bounds = {0};
    CalculateTotalBounds(topArts, bounds);
    double cx = (bounds.left + bounds.right) / 2.0;
    double cy = (bounds.top + bounds.bottom) / 2.0;

    AIRealMatrix matrix;
    if (horizontalAxis) {
        // Reflect across horizontal axis (flip top <-> bottom)
        matrix.a = 1.0;
        matrix.b = 0.0;
        matrix.c = 0.0;
        matrix.d = -1.0;
        matrix.tx = 0.0;
        matrix.ty = 2.0 * cy;
        if (sAIUndo) {
            sAIUndo->SetUndoTextUS(ai::UnicodeString("Reflect Horizontal"), ai::UnicodeString("Undo Reflect Horizontal"));
        }
    } else {
        // Reflect across vertical axis (flip left <-> right)
        matrix.a = -1.0;
        matrix.b = 0.0;
        matrix.c = 0.0;
        matrix.d = 1.0;
        matrix.tx = 2.0 * cx;
        matrix.ty = 0.0;
        if (sAIUndo) {
            sAIUndo->SetUndoTextUS(ai::UnicodeString("Reflect Vertical"), ai::UnicodeString("Undo Reflect Vertical"));
        }
    }

    for (AIArtHandle src : topArts) {
        if (!src) continue;
        TransformArtRecursive(src, matrix);
    }

    sAIDocument->RedrawDocument();
    PluginLog("[ReflectSelection SUCCESS] Reflected %zu item(s) (axis=%s, cx=%.2f, cy=%.2f)",
              topArts.size(), horizontalAxis ? "Horizontal" : "Vertical", cx, cy);
    return true;
}



