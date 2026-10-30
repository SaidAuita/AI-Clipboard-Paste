#include "BufferManager.h"
#include "ConfigManager.h"
#include "IAICopyScope.h"
#include <cstring>
#include <cmath>
#include <algorithm>

void BufferManager::SelectArtRecursive(AIArtHandle art, bool select) {
    if (!art || !sAIArt) return;

    ai::int32 attr = select ? (kArtSelected | kArtFullySelected) : 0;
    ai::int32 mask = kArtSelected | kArtFullySelected;
    sAIArt->SetArtUserAttr(art, mask, attr);

    short type = 0;
    if (sAIArt->GetArtType(art, &type) == kNoErr) {
        if (type == kGroupArt || type == kCompoundPathArt) {
            AIArtHandle child = nullptr;
            if (sAIArt->GetArtFirstChild(art, &child) == kNoErr) {
                while (child) {
                    SelectArtRecursive(child, select);
                    AIArtHandle next = nullptr;
                    if (sAIArt->GetArtSibling(child, &next) == kNoErr) {
                        child = next;
                    } else {
                        break;
                    }
                }
            }
        }
    }
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

void BufferManager::OffsetArt(AIArtHandle art, double dx, double dy) {
    if (!sAIArt || !sAITransformArt || (std::abs(dx) < 1e-6 && std::abs(dy) < 1e-6)) return;
    if (!art) return;

    short type = 0;
    if (sAIArt->GetArtType(art, &type) != kNoErr) return;

    // Groups and compound paths require recursive traversal to transform member artwork
    if (type == kGroupArt || type == kCompoundPathArt) {
        AIArtHandle child = nullptr;
        if (sAIArt->GetArtFirstChild(art, &child) == kNoErr) {
            while (child) {
                OffsetArt(child, dx, dy);
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

    AIRealMatrix matrix;
    matrix.a = 1.0;
    matrix.b = 0.0;
    matrix.c = 0.0;
    matrix.d = 1.0;
    matrix.tx = dx;
    matrix.ty = dy;

    ai::int32 flags = kTransformObjects | kTransformChildren | kTransformEntireArtStyle;
    AIErr transErr = sAITransformArt->TransformArt(art, &matrix, 1.0, flags);
    if (transErr != kNoErr) {
        PluginLog("  [OffsetArt WARN] TransformArt err=%d for art %p (type=%d)", transErr, art, type);
    }
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
    MessageBeep(MB_OK);
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

    // Применяем выделение к вставленным объектам (согласно настройкам)
    bool selectAfter = ConfigManager::GetInstance().GetSelectAfterPaste();
    for (AIArtHandle item : newItems) {
        SelectArtRecursive(item, selectAfter);
    }

    sAIDocument->RedrawDocument();
    MessageBeep(MB_OK);
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
        PluginLog("[PasteCenter FAIL] No items could be duplicated");
        return false;
    }

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
    for (AIArtHandle item : newItems) {
        SelectArtRecursive(item, selectAfter);
    }

    sAIDocument->RedrawDocument();
    MessageBeep(MB_OK);
    PluginLog("[PasteCenter SUCCESS] Duplicated %zu item(s) to view center (dx=%.2f, dy=%.2f, isSameDoc=%d)",
              newItems.size(), dx, dy, isSameDoc);
    return true;
}
