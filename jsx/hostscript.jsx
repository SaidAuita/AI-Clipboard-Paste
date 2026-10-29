#targetengine "CP_SHARED"

/**
 * Clipboard Paste — HostScript (ExtendScript)
 * Высокоскоростное ядро дублирования объектов в обход системного буфера Windows
 * Версия: 1.0.12
 */

// ES3 JSON polyfill with Unicode escape for Adobe Illustrator CEP bridge
if (typeof JSON === "undefined" || !JSON || !JSON.__cpJsonReady) {
    (function() {
        function escStr(s) {
            return '"' + String(s).replace(/\\/g, '\\\\')
                                  .replace(/"/g, '\\"')
                                  .replace(/\n/g, '\\n')
                                  .replace(/\r/g, '\\r')
                                  .replace(/\t/g, '\\t')
                                  .replace(/[\u007F-\uFFFF]/g, function(c) {
                                      return '\\u' + ('0000' + c.charCodeAt(0).toString(16)).slice(-4);
                                  }) + '"';
        }

        function stringifyValue(val) {
            if (val === null) return 'null';
            var t = typeof val;
            if (t === 'undefined' || t === 'function') return undefined;
            if (t === 'number') return isFinite(val) ? String(val) : 'null';
            if (t === 'boolean') return String(val);
            if (t === 'string') return escStr(val);

            var isArr = (val instanceof Array || (val && val.constructor === Array) || Object.prototype.toString.call(val) === '[object Array]');
            if (isArr) {
                var arr = [];
                for (var i = 0; i < val.length; i++) {
                    var itemVal = stringifyValue(val[i]);
                    arr.push(itemVal === undefined ? 'null' : itemVal);
                }
                return '[' + arr.join(',') + ']';
            }

            var props = [];
            for (var k in val) {
                if (Object.prototype.hasOwnProperty.call(val, k)) {
                    var pVal = stringifyValue(val[k]);
                    if (pVal !== undefined) {
                        props.push(escStr(k) + ':' + pVal);
                    }
                }
            }
            return '{' + props.join(',') + '}';
        }

        JSON = {
            __cpJsonReady: true,
            stringify: function(obj) {
                var res = stringifyValue(obj);
                return res === undefined ? 'undefined' : res;
            },
            parse: function(str) {
                return eval('(' + str + ')');
            }
        };
    })();
}

if (typeof $.global.CP_SESSION_BUFFER === "undefined") {
    $.global.CP_SESSION_BUFFER = null;
}

$.global.ClipboardPasteHost = {
        // Внутренний буфер сессии плагина (связан с глобальной памятью ExtendScript)
        getBuffer: function() {
            var b = $.global.CP_SESSION_BUFFER || this.buffer;
            if (!b) {
                try { b = app.__CP_BUFFER__; } catch(eApp) {}
            }
            return b;
        },
        setBuffer: function(buf) {
            $.global.CP_SESSION_BUFFER = buf;
            this.buffer = buf;
            try { app.__CP_BUFFER__ = buf; } catch(eApp) {}
        },
        buffer: null,

        /**
         * Шаг А: Скопировать выделенные объекты в буфер плагина
         */
        copyToBuffer: function() {
        try {
            if (app.documents.length === 0) {
                return JSON.stringify({ success: false, error: "NO_DOCUMENTS" });
            }

            var doc = app.activeDocument;
            var rawSel = doc.selection;

            if (!rawSel || rawSel.length === 0) {
                return JSON.stringify({ success: false, error: "NO_SELECTION" });
            }

            var sel = [];
            var minL = Infinity, maxT = -Infinity, maxR = -Infinity, minB = Infinity;

            for (var s = 0; s < rawSel.length; s++) {
                var itm = rawSel[s];
                sel.push(itm);
                var b = itm.geometricBounds;
                if (b[0] < minL) minL = b[0];
                if (b[1] > maxT) maxT = b[1];
                if (b[2] > maxR) maxR = b[2];
                if (b[3] < minB) minB = b[3];
            }

            var ab = doc.artboards[doc.artboards.getActiveArtboardIndex()];

            var newBuf = {
                srcDoc: doc,
                docName: doc.name,
                items: sel,
                count: sel.length,
                bounds: [minL, maxT, maxR, minB],
                artboardRect: ab.artboardRect
            };
            this.setBuffer(newBuf);

            // Мгновенное оповещение CEP-панели через CSXSEvent
            try {
                var xLib = new ExternalObject("lib:PlugPlugExternalObject");
                if (xLib) {
                    var eventObj = new CSXSEvent();
                    eventObj.type = "com.clipboardpaste.bufferChanged";
                    eventObj.data = '{"count":' + sel.length + ',"docName":"' + doc.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"}';
                    eventObj.dispatch();
                }
            } catch (eEvt) {}

            return JSON.stringify({
                success: true,
                count: sel.length,
                docName: doc.name
            });
        } catch (e) {
            return JSON.stringify({ success: false, error: e.message || String(e) });
        }
    },

    /**
     * Получить статус текущего буфера
     */
    getBufferInfo: function() {
        try {
            var buf = this.getBuffer();
            if (!buf || !buf.items || buf.items.length === 0) {
                return JSON.stringify({ hasBuffer: false, count: 0, docName: "" });
            }

            // Проверка, живы ли объекты (не закрыт ли документ)
            try {
                var test = buf.items[0].name;
            } catch (errAlive) {
                this.setBuffer(null);
                return JSON.stringify({ hasBuffer: false, count: 0, docName: "", error: "DOC_CLOSED" });
            }

            return JSON.stringify({
                hasBuffer: true,
                count: buf.count,
                docName: buf.docName
            });
        } catch (e) {
            return JSON.stringify({ hasBuffer: false, count: 0, docName: "", error: e.message });
        }
    },

    /**
     * Шаг B: Вставить из буфера в текущий активный документ и слой
     * @param {string} placement: "IN_PLACE" (Ctrl+F) или "CENTER" (Ctrl+V)
     */
    pasteFromBuffer: function(placement) {
        try {
            if (app.documents.length === 0) {
                return JSON.stringify({ success: false, error: "NO_DOCUMENTS" });
            }

            var buf = this.getBuffer();
            if (!buf || !buf.items || buf.items.length === 0) {
                var diag = "gB=" + ($.global.CP_SESSION_BUFFER ? "OK" : "null") + ",appB=" + (typeof app !== "undefined" && app.__CP_BUFFER__ ? "OK" : "null");
                return JSON.stringify({ success: false, error: "BUFFER_EMPTY", diag: diag });
            }

            // Проверка жизнеспособности исходных объектов
            try {
                var test = buf.items[0].typename;
            } catch (errClosed) {
                this.setBuffer(null);
                return JSON.stringify({ success: false, error: "DOC_CLOSED" });
            }

            var destDoc = app.activeDocument;
            var targetLayer = destDoc.activeLayer;

            if (targetLayer.locked) targetLayer.locked = false;
            if (!targetLayer.visible) targetLayer.visible = true;

            var destAb = destDoc.artboards[destDoc.artboards.getActiveArtboardIndex()];
            var destAbRect = destAb.artboardRect;

            var dx = 0;
            var dy = 0;

            if (placement === "CENTER") {
                var selBounds = buf.bounds;
                var selMidX = (selBounds[0] + selBounds[2]) / 2;
                var selMidY = (selBounds[1] + selBounds[3]) / 2;

                var destMidX = (destAbRect[0] + destAbRect[2]) / 2;
                var destMidY = (destAbRect[1] + destAbRect[3]) / 2;

                dx = destMidX - selMidX;
                dy = destMidY - selMidY;
            } else {
                // IN_PLACE
                if (destDoc === buf.srcDoc) {
                    dx = 0;
                    dy = 0;
                } else {
                    dx = destAbRect[0] - buf.artboardRect[0];
                    dy = destAbRect[1] - buf.artboardRect[1];
                }
            }

            destDoc.selection = null;
            var newSel = [];
            var copiedCount = 0;

            for (var i = 0; i < buf.items.length; i++) {
                var item = buf.items[i];
                var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
                dup.left = item.left + dx;
                dup.top = item.top + dy;
                newSel.push(dup);
                copiedCount++;
            }

            destDoc.selection = newSel;

            // Мгновенное оповещение CEP-панели через CSXSEvent
            try {
                var xLib = new ExternalObject("lib:PlugPlugExternalObject");
                if (xLib) {
                    var eventObj = new CSXSEvent();
                    eventObj.type = "com.clipboardpaste.bufferChanged";
                    eventObj.data = '{"count":' + newSel.length + ',"docName":"' + destDoc.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"}';
                    eventObj.dispatch();
                }
            } catch (eEvt) {}

            return JSON.stringify({
                success: true,
                count: copiedCount,
                docName: destDoc.name,
                layerName: targetLayer.name,
                mode: placement || "IN_PLACE"
            });
        } catch (e) {
            return JSON.stringify({ success: false, error: e.message || String(e) });
        }
    },

    /**
     * Получить список слоев и открытых документов (для вкладки 1 клик)
     */
    getLayersAndDocsList: function() {
        try {
            if (app.documents.length === 0) {
                return JSON.stringify({
                    success: true,
                    hasDoc: false,
                    docCount: 0,
                    docName: "",
                    selectionCount: 0,
                    currentLayerName: "",
                    layers: [],
                    documents: []
                });
            }

            var activeDoc = app.activeDocument;
            var layersResult = [];

            for (var i = 0; i < activeDoc.layers.length; i++) {
                var l = activeDoc.layers[i];
                layersResult.push({
                    name: l.name,
                    isLocked: l.locked,
                    isVisible: l.visible,
                    isCurrent: (l === activeDoc.activeLayer)
                });
            }

            var docsResult = [];
            for (var d = 0; d < app.documents.length; d++) {
                var docItem = app.documents[d];
                docsResult.push({
                    index: d,
                    name: docItem.name,
                    isCurrent: (docItem === activeDoc)
                });
            }

            var selCount = activeDoc.selection ? activeDoc.selection.length : 0;

            return JSON.stringify({
                success: true,
                hasDoc: true,
                docCount: app.documents.length,
                docName: activeDoc.name,
                selectionCount: selCount,
                currentLayerName: activeDoc.activeLayer ? activeDoc.activeLayer.name : "",
                layers: layersResult,
                documents: docsResult
            });
        } catch (e) {
            return JSON.stringify({ success: false, error: e.message || String(e) });
        }
    },

    getLayersList: function() {
        return this.getLayersAndDocsList();
    },

    /**
     * Вставить на целевой слой в то же самое место (In Place) или по центру
     */
    pasteToTargetLayer: function(mode, targetLayerName, placement) {
        try {
            if (app.documents.length === 0) {
                return JSON.stringify({ success: false, error: "NO_DOCUMENTS" });
            }

            var doc = app.activeDocument;
            var rawSel = doc.selection;

            if (!rawSel || rawSel.length === 0) {
                return JSON.stringify({ success: false, error: "NO_SELECTION" });
            }

            var sel = [];
            for (var s = 0; s < rawSel.length; s++) {
                sel.push(rawSel[s]);
            }

            var targetLayer = null;

            if (mode === "NEW") {
                var srcLayer = doc.activeLayer;
                targetLayer = doc.layers.add();
                targetLayer.name = srcLayer ? (srcLayer.name + " Copy") : "Layer Copy";
                if (srcLayer) {
                    try {
                        targetLayer.move(srcLayer, ElementPlacement.PLACEBEFORE);
                    } catch (e) {}
                }
            } else if (mode === "BY_NAME" && targetLayerName) {
                try {
                    targetLayer = doc.layers.getByName(targetLayerName);
                } catch (e) {
                    targetLayer = doc.layers.add();
                    targetLayer.name = targetLayerName;
                }
            } else {
                targetLayer = doc.layers.add();
                targetLayer.name = "Copy_Layer";
            }

            if (targetLayer.locked) targetLayer.locked = false;
            if (!targetLayer.visible) targetLayer.visible = true;

            var dx = 0;
            var dy = 0;
            if (placement === "CENTER") {
                var minLeft = Infinity, maxTop = -Infinity, maxRight = -Infinity, minBottom = Infinity;
                for (var bIdx = 0; bIdx < sel.length; bIdx++) {
                    var gb = sel[bIdx].geometricBounds;
                    if (gb[0] < minLeft) minLeft = gb[0];
                    if (gb[1] > maxTop) maxTop = gb[1];
                    if (gb[2] > maxRight) maxRight = gb[2];
                    if (gb[3] < minBottom) minBottom = gb[3];
                }
                var selCenterX = (minLeft + maxRight) / 2;
                var selCenterY = (maxTop + minBottom) / 2;

                var ab = doc.artboards[doc.artboards.getActiveArtboardIndex()];
                var abRect = ab.artboardRect;
                var abCenterX = (abRect[0] + abRect[2]) / 2;
                var abCenterY = (abRect[1] + abRect[3]) / 2;

                dx = abCenterX - selCenterX;
                dy = abCenterY - selCenterY;
            }

            doc.selection = null;
            var newSel = [];
            var copiedCount = 0;

            for (var i = 0; i < sel.length; i++) {
                var item = sel[i];
                var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
                dup.left = item.left + dx;
                dup.top = item.top + dy;
                newSel.push(dup);
                copiedCount++;
            }

            doc.activeLayer = targetLayer;
            doc.selection = newSel;

            return JSON.stringify({
                success: true,
                count: copiedCount,
                layerName: targetLayer.name
            });
        } catch (e) {
            return JSON.stringify({ success: false, error: e.message || String(e) });
        }
    },

    pasteInPlaceTargetLayer: function(mode, targetLayerName) {
        return this.pasteToTargetLayer(mode, targetLayerName, "IN_PLACE");
    },

    /**
     * Вставить в другой открытый документ на его активный слой
     */
    pasteToDocument: function(targetDocId, placement) {
        try {
            if (app.documents.length === 0) {
                return JSON.stringify({ success: false, error: "NO_DOCUMENTS" });
            }

            if (app.documents.length < 2) {
                return JSON.stringify({ success: false, error: "ONLY_ONE_DOC" });
            }

            var srcDoc = app.activeDocument;
            var rawSel = srcDoc.selection;

            if (!rawSel || rawSel.length === 0) {
                return JSON.stringify({ success: false, error: "NO_SELECTION" });
            }

            var sel = [];
            for (var s = 0; s < rawSel.length; s++) {
                sel.push(rawSel[s]);
            }

            var targetDoc = null;
            if (typeof targetDocId === "number") {
                if (targetDocId >= 0 && targetDocId < app.documents.length) {
                    targetDoc = app.documents[targetDocId];
                }
            } else if (typeof targetDocId === "string" && targetDocId.length > 0) {
                try {
                    targetDoc = app.documents.getByName(targetDocId);
                } catch (e) {
                    for (var d = 0; d < app.documents.length; d++) {
                        if (app.documents[d].name === targetDocId) {
                            targetDoc = app.documents[d];
                            break;
                        }
                    }
                }
            }

            if (!targetDoc || targetDoc === srcDoc) {
                for (var k = 0; k < app.documents.length; k++) {
                    if (app.documents[k] !== srcDoc) {
                        targetDoc = app.documents[k];
                        break;
                    }
                }
            }

            if (!targetDoc) {
                return JSON.stringify({ success: false, error: "TARGET_DOC_NOT_FOUND" });
            }

            var srcAb = srcDoc.artboards[srcDoc.artboards.getActiveArtboardIndex()];
            var srcAbRect = srcAb.artboardRect;

            var destAb = targetDoc.artboards[targetDoc.artboards.getActiveArtboardIndex()];
            var destAbRect = destAb.artboardRect;

            var targetLayer = targetDoc.activeLayer;
            if (targetLayer.locked) targetLayer.locked = false;
            if (!targetLayer.visible) targetLayer.visible = true;

            var dx = 0;
            var dy = 0;

            if (placement === "CENTER") {
                var minL = Infinity, maxT = -Infinity, maxR = -Infinity, minB = Infinity;
                for (var b = 0; b < sel.length; b++) {
                    var bounds = sel[b].geometricBounds;
                    if (bounds[0] < minL) minL = bounds[0];
                    if (bounds[1] > maxT) maxT = bounds[1];
                    if (bounds[2] > maxR) maxR = bounds[2];
                    if (bounds[3] < minB) minB = bounds[3];
                }
                var selMidX = (minL + maxR) / 2;
                var selMidY = (maxT + minB) / 2;

                var destMidX = (destAbRect[0] + destAbRect[2]) / 2;
                var destMidY = (destAbRect[1] + destAbRect[3]) / 2;

                dx = destMidX - selMidX;
                dy = destMidY - selMidY;
            } else {
                dx = destAbRect[0] - srcAbRect[0];
                dy = destAbRect[1] - srcAbRect[1];
            }

            targetDoc.selection = null;
            var newSel = [];
            var copiedCount = 0;

            for (var m = 0; m < sel.length; m++) {
                var item = sel[m];
                var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
                dup.left = item.left + dx;
                dup.top = item.top + dy;
                newSel.push(dup);
                copiedCount++;
            }

            targetDoc.selection = newSel;
            targetDoc.activate();

            return JSON.stringify({
                success: true,
                count: copiedCount,
                docName: targetDoc.name,
                layerName: targetLayer.name,
                mode: placement || "IN_PLACE"
            });
        } catch (e) {
            return JSON.stringify({ success: false, error: e.message || String(e) });
        }
    }
};

var ClipboardPasteHost = $.global.ClipboardPasteHost;
