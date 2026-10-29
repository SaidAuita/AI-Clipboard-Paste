const fs = require("fs");
const path = require("path");
const { execSync } = require("child_process");

const ROOT_DIR = __dirname;
const BUILD_DEV_DIR = path.join(ROOT_DIR, "Build_DEV");
const DIST_DIR = path.join(ROOT_DIR, "dist", "Clipboard Paste");
const APPDATA_CEP = path.join(
    process.env.APPDATA || "C:\\Users\\said\\AppData\\Roaming",
    "Adobe", "CEP", "extensions", "Clipboard Paste"
);

function getVersion() {
    const manifestPath = path.join(ROOT_DIR, "CSXS", "manifest.xml");
    if (!fs.existsSync(manifestPath)) return "1.0.0";
    const content = fs.readFileSync(manifestPath, "utf8");
    const m = content.match(/ExtensionBundleVersion="([^"]+)"/);
    return m ? m[1] : "1.0.0";
}

function copyDirClean(src, dst, excludes = []) {
    if (!fs.existsSync(src)) return;
    fs.mkdirSync(dst, { recursive: true });
    const entries = fs.readdirSync(src, { withFileTypes: true });
    for (const ent of entries) {
        if (excludes.includes(ent.name) || ent.name.startsWith("~") || ent.name.endsWith(".tmp") || ent.name.endsWith(".zip")) {
            continue;
        }
        const srcPath = path.join(src, ent.name);
        const dstPath = path.join(dst, ent.name);
        if (ent.isDirectory()) {
            copyDirClean(srcPath, dstPath, excludes);
        } else {
            fs.copyFileSync(srcPath, dstPath);
        }
    }
}

function zipFolder(srcDir, zipPath) {
    fs.mkdirSync(path.dirname(zipPath), { recursive: true });
    if (fs.existsSync(zipPath)) {
        try { fs.unlinkSync(zipPath); } catch (e) {}
    }

    const psCommand = `powershell -NoProfile -Command "Compress-Archive -Path '${srcDir}\\*' -DestinationPath '${zipPath}' -Force"`;
    execSync(psCommand, { stdio: "inherit" });
}

console.log("================================================================================");
console.log("       CLIPBOARD PASTE - DEV BUILD & CEP SYNC SCRIPT");
console.log("================================================================================");

const version = getVersion();
console.log(`[*] Version detected: v${version}`);

const excludes = [
    ".git", ".gitignore", "Build_DEV", "dist", "build_dev.js", "build_dev.bat", "node_modules", "scratch", ".doc", "generate_icons.py", "images", "tools"
];

// 1. Sync to dist/Clipboard Paste
console.log("[1/3] Mirroring to dist/Clipboard Paste/...");
if (fs.existsSync(DIST_DIR)) {
    fs.rmSync(DIST_DIR, { recursive: true, force: true });
}
copyDirClean(ROOT_DIR, DIST_DIR, excludes);

// 2. Sync to %APPDATA%\Adobe\CEP\extensions\Clipboard Paste
console.log("[2/3] Syncing directly to Adobe CEP extensions directory...");
copyDirClean(ROOT_DIR, APPDATA_CEP, excludes);
console.log(`  -> Synced to: ${APPDATA_CEP}`);

// 3. Package to Build_DEV/Clipboard_Paste_v<VERSION>.zip
console.log(`[3/3] Packing clean DEV archive...`);
const zipName = `Clipboard_Paste_v${version}.zip`;
const zipPath = path.join(BUILD_DEV_DIR, zipName);
zipFolder(DIST_DIR, zipPath);
console.log(`  -> Archive created: ${zipPath}`);

console.log("\n[SUCCESS] Build & Sync Complete!");
