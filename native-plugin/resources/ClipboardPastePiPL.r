#define PlugInVersion 1
#define PlugInName "Clipboard Paste"

resource 'PiPL' (16000) {
    {
        Kind { Filter },
        Name { "Clipboard Paste" },
        Version { (PlugInVersion << 16) },
        Component { 'SP64', "PluginMain" },
        EnableInfo { "true" }
    }
};
