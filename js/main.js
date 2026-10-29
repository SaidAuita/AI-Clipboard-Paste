(function() {
    "use strict";

    var cs = window.csInterface || (typeof CSInterface !== "undefined" ? new CSInterface() : null);
    if (cs && cs.evalScript) {
        var _origEvalScript = cs.evalScript.bind(cs);
        cs.evalScript = function(script, callback) {
            if (typeof script === "string" && script.indexOf("#targetengine") !== 0) {
                script = '#targetengine "CP_SHARED"\n' + script;
            }
            return _origEvalScript(script, callback);
        };
    }

    // ==========================================
    // ЛОКАЛИЗАЦИЯ (8 ЯЗЫКОВ ИЗ js/locales.js)
    // ==========================================
    var locales = window.CP_LOCALES || {};

    var currentLang = localStorage.getItem("cp_lang") || "ru";
    if (!locales[currentLang]) currentLang = "ru";

    function setLanguage(lang) {
        if (!locales[lang]) return;
        currentLang = lang;
        localStorage.setItem("cp_lang", lang);

        var langSelect = document.getElementById("lang_select");
        if (langSelect && langSelect.value !== lang) {
            langSelect.value = lang;
        }

        var dict = locales[lang];
        var elements = document.querySelectorAll("[data-i18n]");
        for (var i = 0; i < elements.length; i++) {
            var el = elements[i];
            var key = el.getAttribute("data-i18n");
            if (dict[key]) {
                el.textContent = dict[key];
            }
        }
        updateBufferBadgeUI();

        // Синхронизация языка с общим движком ExtendScript и микро-панелями
        if (cs) {
            cs.evalScript('#targetengine "CP_SHARED"\n$.global.CP_LANG = "' + lang + '";');
            try {
                var evt = new CSXSEvent();
                evt.type = "com.clipboardpaste.langChanged";
                evt.data = lang;
                evt.dispatch();
            } catch (eEvt) {}
        }
    }

    // ==========================================
    // ЭЛЕМЕНТЫ UI
    // ==========================================
    var langSelect = document.getElementById("lang_select");
    var tabBtnBuffer = document.getElementById("tabBtnBuffer");
    var tabBtnDirect = document.getElementById("tabBtnDirect");
    var tabBuffer = document.getElementById("tab-buffer");
    var tabDirect = document.getElementById("tab-direct");

    // Вкладка 1 (Буфер)
    var btnBufferCopy = document.getElementById("btnBufferCopy");
    var btnBufferPastePlace = document.getElementById("btnBufferPastePlace");
    var btnBufferPasteCenter = document.getElementById("btnBufferPasteCenter");
    var bufferStatusDot = document.getElementById("bufferStatusDot");
    var bufferStatusText = document.getElementById("bufferStatusText");

    // Вкладка 2 (Списки)
    var selTargetLayer = document.getElementById("selTargetLayer");
    var btnRefresh = document.getElementById("btnRefresh");
    var btnPasteLayer = document.getElementById("btnPasteLayer");
    var selTargetDoc = document.getElementById("selTargetDoc");
    var btnPasteDocPlace = document.getElementById("btnPasteDocPlace");
    var btnPasteDocCenter = document.getElementById("btnPasteDocCenter");

    // Статус-бар
    var statusText = document.getElementById("statusText");
    var statusDot = document.getElementById("statusDot");

    // Ссылки подвала
    var linkGithub = document.getElementById("link_github");
    var linkTools = document.getElementById("link_tools");

    if (langSelect) {
        langSelect.addEventListener("change", function() {
            setLanguage(this.value);
        });
    }

    function openExternalUrl(url) {
        try {
            if (window.cep && window.cep.util && typeof window.cep.util.openURLInDefaultBrowser === "function") {
                window.cep.util.openURLInDefaultBrowser(url);
            } else if (cs && typeof cs.openURLInDefaultBrowser === "function") {
                cs.openURLInDefaultBrowser(url);
            } else {
                window.open(url, "_blank");
            }
        } catch (e) {
            try {
                window.open(url, "_blank");
            } catch (err) {}
        }
    }

    if (linkGithub) {
        linkGithub.addEventListener("click", function(e) {
            e.preventDefault();
            openExternalUrl("https://github.com/SaidAuita/AI-Clipboard-Paste");
        });
    }

    if (linkTools) {
        linkTools.addEventListener("click", function(e) {
            e.preventDefault();
            openExternalUrl("https://ph-cu-s.com/tools");
        });
    }

    function setStatus(msg, isSuccess, isError) {
        if (!statusText || !statusDot) return;
        statusText.textContent = msg;
        if (isError) {
            statusDot.style.backgroundColor = "#ff4757";
        } else if (isSuccess) {
            statusDot.style.backgroundColor = "#00d26a";
        } else {
            statusDot.style.backgroundColor = "#0082ea";
        }
    }

    function translateError(code) {
        var dict = locales[currentLang] || locales.en;
        if (code === "NO_DOCUMENTS") return dict.errNoDoc;
        if (code === "NO_SELECTION") return dict.errNoSel;
        if (code === "BUFFER_EMPTY") return dict.errBufEmpty;
        if (code === "DOC_CLOSED") return dict.errDocClosed;
        if (code === "ONLY_ONE_DOC") return dict.errOnlyOneDoc;
        return code || "Error";
    }

    // ==========================================
    // ПЕРЕКЛЮЧЕНИЕ ВКЛАДОК
    // ==========================================
    function switchTab(tabId) {
        if (tabId === "tab-buffer") {
            tabBtnBuffer.classList.add("active");
            tabBtnDirect.classList.remove("active");
            tabBuffer.classList.add("active");
            tabDirect.classList.remove("active");
        } else {
            tabBtnDirect.classList.add("active");
            tabBtnBuffer.classList.remove("active");
            tabDirect.classList.add("active");
            tabBuffer.classList.remove("active");
            refreshList();
        }
        localStorage.setItem("cp_active_tab", tabId);
    }

    tabBtnBuffer.addEventListener("click", function() { switchTab("tab-buffer"); });
    tabBtnDirect.addEventListener("click", function() { switchTab("tab-direct"); });

    if (langSelect) {
        langSelect.addEventListener("change", function() {
            setLanguage(langSelect.value);
        });
    }

    // ==========================================
    // ЛОГИКА БУФЕРА (ВКЛАДКА 1)
    // ==========================================
    var currentBufferInfo = { hasBuffer: false, count: 0, docName: "" };

    function updateBufferBadgeUI() {
        var dict = locales[currentLang] || locales.en;
        if (currentBufferInfo.hasBuffer) {
            bufferStatusDot.classList.add("active");
            var text = dict.bufferReady.replace("{count}", currentBufferInfo.count).replace("{doc}", currentBufferInfo.docName);
            bufferStatusText.textContent = text;
        } else {
            bufferStatusDot.classList.remove("active");
            bufferStatusText.textContent = dict.bufferEmpty;
        }
    }

    var lastNotifiedCount = -1;
    var lastNotifiedDoc = "";

    function checkBufferStatus() {
        if (!cs) return;
        cs.evalScript("ClipboardPasteHost.getBufferInfo()", function(result) {
            try {
                if (!result || result === "EvalScript error.") return;
                var data = JSON.parse(result);
                currentBufferInfo = data;
                updateBufferBadgeUI();

                if (data.hasBuffer && (data.count !== lastNotifiedCount || data.docName !== lastNotifiedDoc)) {
                    lastNotifiedCount = data.count;
                    lastNotifiedDoc = data.docName;
                    var dict = locales[currentLang] || locales.en;
                    var msg = dict.copiedSuccess.replace("{count}", data.count).replace("{doc}", data.docName);
                    setStatus(msg, true, false);
                } else if (!data.hasBuffer && lastNotifiedCount > 0) {
                    lastNotifiedCount = 0;
                    lastNotifiedDoc = "";
                }
            } catch (e) {}
        });
    }

    btnBufferCopy.addEventListener("click", function() {
        if (!cs) return;
        setStatus("Копирование в буфер...", false, false);
        cs.evalScript("ClipboardPasteHost.copyToBuffer()", function(result) {
            try {
                if (!result || result === "EvalScript error." || result.indexOf("ERR:") === 0 || result.indexOf("Error") === 0) {
                    setStatus("⚠️ " + (result || "Ошибка вызова скрипта"), false, true);
                    return;
                }
                var data = JSON.parse(result);
                if (data.success) {
                    currentBufferInfo = { hasBuffer: true, count: data.count, docName: data.docName };
                    updateBufferBadgeUI();
                    var dict = locales[currentLang] || locales.en;
                    var msg = dict.copiedSuccess.replace("{count}", data.count).replace("{doc}", data.docName);
                    setStatus(msg, true, false);
                } else {
                    setStatus("⚠️ " + translateError(data.error), false, true);
                }
            } catch (e) {
                setStatus("Ошибка: " + e.message, false, true);
            }
        });
    });

    function pasteFromBuffer(mode) {
        if (!cs) return;
        localStorage.setItem("cp_last_paste_mode", mode);
        updatePasteModeHighlight(mode);

        setStatus("Вставка из буфера...", false, false);
        cs.evalScript('ClipboardPasteHost.pasteFromBuffer("' + mode + '")', function(result) {
            try {
                if (!result || result === "EvalScript error." || result.indexOf("ERR:") === 0 || result.indexOf("Error") === 0) {
                    setStatus("⚠️ " + (result || "Ошибка вызова скрипта"), false, true);
                    return;
                }
                var data = JSON.parse(result);
                if (data.success) {
                    var dict = locales[currentLang] || locales.en;
                    var modeName = (data.mode === "CENTER") ? "Ctrl+V" : "In Place (Ctrl+F)";
                    var msg = dict.pastedSuccess.replace("{count}", data.count).replace("{mode}", modeName).replace("{doc}", data.docName);
                    setStatus(msg, true, false);
                } else {
                    setStatus("⚠️ " + translateError(data.error), false, true);
                    if (data.error === "DOC_CLOSED" || data.error === "BUFFER_EMPTY") {
                        currentBufferInfo = { hasBuffer: false, count: 0, docName: "" };
                        updateBufferBadgeUI();
                    }
                }
            } catch (e) {
                setStatus("Ошибка: " + e.message, false, true);
            }
        });
    }

    function updatePasteModeHighlight(mode) {
        if (mode === "CENTER") {
            btnBufferPasteCenter.classList.add("btn-selected-mode");
            btnBufferPastePlace.classList.remove("btn-selected-mode");
        } else {
            btnBufferPastePlace.classList.add("btn-selected-mode");
            btnBufferPasteCenter.classList.remove("btn-selected-mode");
        }
    }

    btnBufferPastePlace.addEventListener("click", function() { pasteFromBuffer("IN_PLACE"); });
    btnBufferPasteCenter.addEventListener("click", function() { pasteFromBuffer("CENTER"); });

    // ==========================================
    // ЛОГИКА 1 КЛИК / СПИСКИ (ВКЛАДКА 2)
    // ==========================================
    function refreshList() {
        if (!cs) return;
        cs.evalScript("ClipboardPasteHost.getLayersAndDocsList()", function(result) {
            try {
                if (!result || result === "EvalScript error." || result.indexOf("Error") === 0) {
                    setStatus("Ошибка хоста: " + result, false, true);
                    return;
                }

                var data = JSON.parse(result);
                if (!data.success) {
                    setStatus("⚠️ " + (data.error || "Неизвестная ошибка"), false, true);
                    return;
                }

                var dict = locales[currentLang] || locales.en;

                if (!data.hasDoc) {
                    selTargetLayer.innerHTML = '<option value="__NEW__">' + dict.newLayerOption + '</option>';
                    selTargetDoc.innerHTML = '<option value="__NONE__">' + dict.docNoneOption + '</option>';
                    setStatus(dict.errNoDoc, false, false);
                    return;
                }

                // 1. Слои
                var prevLayer = selTargetLayer.value;
                selTargetLayer.innerHTML = '<option value="__NEW__">' + dict.newLayerOption + '</option>';

                for (var i = 0; i < data.layers.length; i++) {
                    var l = data.layers[i];
                    var opt = document.createElement("option");
                    opt.value = l.name;
                    var label = l.name;
                    if (l.isCurrent) label += " (Текущий)";
                    if (l.isLocked) label += " 🔒";
                    if (!l.isVisible) label += " 👁️✕";
                    opt.textContent = label;
                    selTargetLayer.appendChild(opt);
                }

                if (prevLayer && prevLayer !== "__NEW__") {
                    for (var j = 0; j < selTargetLayer.options.length; j++) {
                        if (selTargetLayer.options[j].value === prevLayer) {
                            selTargetLayer.value = prevLayer;
                            break;
                        }
                    }
                }

                // 2. Документы
                var prevDoc = selTargetDoc.value;
                selTargetDoc.innerHTML = "";

                var otherDocs = [];
                for (var d = 0; d < data.documents.length; d++) {
                    var docItem = data.documents[d];
                    if (!docItem.isCurrent) {
                        otherDocs.push(docItem);
                    }
                }

                if (otherDocs.length === 0) {
                    var optNone = document.createElement("option");
                    optNone.value = "__NONE__";
                    optNone.textContent = dict.docNoneOption;
                    selTargetDoc.appendChild(optNone);
                } else {
                    var optNext = document.createElement("option");
                    optNext.value = "__NEXT__";
                    optNext.textContent = dict.docNextOption;
                    selTargetDoc.appendChild(optNext);

                    for (var k = 0; k < otherDocs.length; k++) {
                        var oDoc = otherDocs[k];
                        var optDoc = document.createElement("option");
                        optDoc.value = oDoc.name;
                        optDoc.textContent = oDoc.name;
                        selTargetDoc.appendChild(optDoc);
                    }

                    if (prevDoc && prevDoc !== "__NEXT__") {
                        for (var m = 0; m < selTargetDoc.options.length; m++) {
                            if (selTargetDoc.options[m].value === prevDoc) {
                                selTargetDoc.value = prevDoc;
                                break;
                            }
                        }
                    }
                }

                setStatus("Макет: " + data.docName + " (Выделено: " + data.selectionCount + ")", false, false);
            } catch (e) {
                setStatus("Ошибка разбора: " + e.message, false, true);
            }
        });
    }

    btnPasteLayer.addEventListener("click", function() {
        if (!cs) return;
        var mode = (selTargetLayer.value === "__NEW__") ? "NEW" : "BY_NAME";
        var layerName = selTargetLayer.value;

        setStatus("Копирование на слой...", false, false);
        var script = 'ClipboardPasteHost.pasteToTargetLayer("' + mode + '", "' + layerName.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '", "IN_PLACE")';
        cs.evalScript(script, function(result) {
            try {
                if (!result || result === "EvalScript error." || result.indexOf("ERR:") === 0 || result.indexOf("Error") === 0) {
                    setStatus("⚠️ " + (result || "Ошибка вызова скрипта"), false, true);
                    return;
                }
                var data = JSON.parse(result);
                if (data.success) {
                    setStatus("✅ Скопировано " + data.count + " объектов на слой \"" + data.layerName + "\"!", true, false);
                    refreshList();
                } else {
                    setStatus("⚠️ " + translateError(data.error), false, true);
                }
            } catch (e) {
                setStatus("Ошибка: " + e.message, false, true);
            }
        });
    });

    function pasteToOtherDocument(placementMode) {
        if (!cs) return;
        var targetDoc = selTargetDoc.value;
        if (targetDoc === "__NONE__") {
            setStatus("⚠️ " + (locales[currentLang] || locales.en).errOnlyOneDoc, false, true);
            return;
        }

        var modeLabel = (placementMode === "CENTER") ? "по центру" : "in place";
        setStatus("Вставка " + modeLabel + " в документ...", false, false);

        var targetParam = (targetDoc === "__NEXT__") ? "" : targetDoc;
        var script = 'ClipboardPasteHost.pasteToDocument("' + targetParam.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '", "' + placementMode + '")';
        cs.evalScript(script, function(result) {
            try {
                if (!result || result === "EvalScript error." || result.indexOf("ERR:") === 0 || result.indexOf("Error") === 0) {
                    setStatus("⚠️ " + (result || "Ошибка вызова скрипта"), false, true);
                    return;
                }
                var data = JSON.parse(result);
                if (data.success) {
                    var actionName = (data.mode === "CENTER") ? "по центру" : "In Place (Ctrl+F)";
                    setStatus("✅ Скопировано " + data.count + " в \"" + data.docName + "\" (" + actionName + ")!", true, false);
                    refreshList();
                } else {
                    setStatus("⚠️ " + translateError(data.error), false, true);
                }
            } catch (e) {
                setStatus("Ошибка: " + e.message, false, true);
            }
        });
    }

    btnPasteDocPlace.addEventListener("click", function() { pasteToOtherDocument("IN_PLACE"); });
    btnPasteDocCenter.addEventListener("click", function() { pasteToOtherDocument("CENTER"); });
    btnRefresh.addEventListener("click", function() { refreshList(); });

    // ==========================================
    // ОБРАБОТЧИК КЛАВИШ ВНУТРИ ПАНЕЛИ
    // ==========================================
    // Клавиши обрабатываются строго при фокусе на панели, без глобального перехвата в Illustrator

    // Обработчик клавиш внутри панели
    window.addEventListener("keydown", function(e) {
        if (e.target && (e.target.tagName === "INPUT" || e.target.tagName === "SELECT")) {
            return;
        }

        var isBufferTab = tabBuffer.classList.contains("active");

        // 1. Ctrl + F5 -> Скопировать в буфер (Шаг А)
        if (e.ctrlKey && (e.key === "F5" || e.keyCode === 116)) {
            e.preventDefault();
            btnBufferCopy.click();
            return;
        }

        // 2. Ctrl + F6 -> Вставить In Place (Шаг B / Ctrl+F)
        if (e.ctrlKey && (e.key === "F6" || e.keyCode === 117)) {
            e.preventDefault();
            if (isBufferTab) {
                btnBufferPastePlace.click();
            } else {
                btnPasteDocPlace.click();
            }
            return;
        }

        // 3. Ctrl + F7 -> Вставить По центру (Шаг B / Ctrl+V)
        if (e.ctrlKey && (e.key === "F7" || e.keyCode === 118)) {
            e.preventDefault();
            if (isBufferTab) {
                btnBufferPasteCenter.click();
            } else {
                btnPasteDocCenter.click();
            }
            return;
        }

        // Переключение вкладок по клавише Tab
        if (e.key === "Tab") {
            e.preventDefault();
            switchTab(isBufferTab ? "tab-direct" : "tab-buffer");
            return;
        }

        // Дополнительные одиночные клавиши при активной вкладке
        if (isBufferTab) {
            // Клавиша 'C' или '1' -> Скопировать
            if (e.key.toLowerCase() === "c" || e.key === "1") {
                e.preventDefault();
                btnBufferCopy.click();
            }
            // Клавиша 'F' или '2' -> Вставить In Place
            else if (e.key.toLowerCase() === "f" || e.key === "2") {
                e.preventDefault();
                btnBufferPastePlace.click();
            }
            // Клавиша 'V' или '3' -> Вставить По центру
            else if (e.key.toLowerCase() === "v" || e.key === "3") {
                e.preventDefault();
                btnBufferPasteCenter.click();
            }
        } else {
            // Клавиша '1' или 'L' -> На слой
            if (e.key === "1" || e.key.toLowerCase() === "l") {
                e.preventDefault();
                btnPasteLayer.click();
            }
            // Клавиша '2' или 'F' -> В документ In Place
            else if (e.key === "2" || e.key.toLowerCase() === "f") {
                e.preventDefault();
                btnPasteDocPlace.click();
            }
            // Клавиша '3' или 'V' -> В документ По центру
            else if (e.key === "3" || e.key.toLowerCase() === "v") {
                e.preventDefault();
                btnPasteDocCenter.click();
            }
        }

        // F5 или 'R' -> Обновить
        if (!e.ctrlKey && (e.key === "F5" || e.key.toLowerCase() === "r")) {
            e.preventDefault();
            checkBufferStatus();
            refreshList();
        }
    });

    // ==========================================
    // ТЕМА ОФОРМЛЕНИЯ
    // ==========================================
    function updateTheme() {
        if (!cs) return;
        try {
            var appSkin = cs.getHostEnvironment().appSkinInfo;
            if (appSkin && appSkin.panelBackgroundColor) {
                var color = appSkin.panelBackgroundColor.color;
                var r = Math.round(color.red);
                var g = Math.round(color.green);
                var b = Math.round(color.blue);
                document.documentElement.style.setProperty("--bg-color", "rgb(" + r + "," + g + "," + b + ")");

                if (r > 128) {
                    document.documentElement.style.setProperty("--panel-bg", "#e8e8e8");
                    document.documentElement.style.setProperty("--card-bg", "#ffffff");
                    document.documentElement.style.setProperty("--text-color", "#1a1a1a");
                    document.documentElement.style.setProperty("--text-muted", "#666666");
                    document.documentElement.style.setProperty("--border-color", "#cccccc");
                }
            }
        } catch (e) {}
    }

    // Загрузка ExtendScript хоста
    function loadHostScript() {
        if (!cs) return;
        var extensionPath = cs.getSystemPath(SystemPath.EXTENSION);
        var hostScriptPath = (extensionPath + "/jsx/hostscript.jsx").replace(/\\/g, "/");
        cs.evalScript('try { if (typeof $.global.ClipboardPasteHost === "undefined" || !$.global.ClipboardPasteHost) { $.evalFile(new File("' + hostScriptPath + '")); } "OK"; } catch(e) { "ERR: " + (e.message || String(e)); }', function(res) {
            checkBufferStatus();
        });
    }

    window.addEventListener("focus", function() {
        checkBufferStatus();
        var isBufferTab = tabBuffer && tabBuffer.classList.contains("active");
        if (!isBufferTab) {
            refreshList();
        }
    });

    // CSXS Event для моментального обновления при выполнении внешних скриптов / горячих клавиш
    if (cs) {
        try {
            cs.addEventListener("com.clipboardpaste.bufferChanged", function(evt) {
                checkBufferStatus();
                var isBufferTab = tabBuffer && tabBuffer.classList.contains("active");
                if (!isBufferTab) {
                    refreshList();
                }
            });
        } catch (eEvt) {}
    }

    // ==========================================
    // СТАРТ И ВОССТАНОВЛЕНИЕ СОСТОЯНИЯ
    // ==========================================
    setLanguage(currentLang);

    var savedTab = localStorage.getItem("cp_active_tab");
    if (savedTab) switchTab(savedTab);

    var savedMode = localStorage.getItem("cp_last_paste_mode");
    if (savedMode) updatePasteModeHighlight(savedMode);

    updateTheme();
    setTimeout(loadHostScript, 300);
})();
