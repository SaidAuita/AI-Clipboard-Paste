#pragma once

#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Suites.h"
#include <vector>

class BufferManager {
public:
    static BufferManager& GetInstance();

    bool CopySelection();
    bool PasteInPlace();
    bool PasteCenter();
    bool PasteAllArtboards();
    void Clear();

    // Reflect methods
    bool ReflectSelectionVertical();
    bool ReflectSelectionHorizontal();
    bool ReflectSelection(bool horizontalAxis);

    // Pattern Offset Clone & Move methods
    bool CloneSelectionWithOffset(int direction, double gap = 0.0);
    bool MoveSelectionWithOffset(int direction, double gap = 0.0);
    bool GetSelectionDimensions(double& outW, double& outH);

    bool HasData() const { return m_hasData; }
    size_t GetItemCount() const { return m_clonedItems.size(); }

private:
    BufferManager();
    ~BufferManager();
    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;

    void CalculateTotalBounds(const std::vector<AIArtHandle>& items, AIRealRect& outRect);
    void TransformArtRecursive(AIArtHandle art, const AIRealMatrix& matrix);
    void OffsetArt(AIArtHandle art, double dx, double dy);
    void SelectArt(AIArtHandle art, bool select);
    bool IsLayerArt(AIArtHandle art);
    bool HasSelectedAncestor(AIArtHandle art, const std::vector<AIArtHandle>& allMatches);
    bool IsSourceDocOpen() const;

    bool m_hasData;
    AIDocumentHandle m_sourceDoc;
    std::vector<AIArtHandle> m_clonedItems;
    AIRealRect m_sourceBounds;
    ai::ArtboardID m_sourceArtboardIndex;
    AIRealRect m_sourceArtboardBounds;
};

#endif // BUFFER_MANAGER_H
