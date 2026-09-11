"""Editor-side half of the text-anomaly texture round trip: import the generated PNGs.

Run headless after make_text_anomaly_textures.py:

    UnrealEditor-Cmd.exe <Loop9.uproject> -EnablePlugins=PythonScriptPlugin
        -ExecutePythonScript="Tools/EditorPython/reimport_anomaly_textures.py"
        -unattended -nopause -nosplash

Every PNG in SRC is imported over the asset of the same name in its anomaly
folder (replace_existing), so the material instances keep their references.
"""
import os
import unreal

SRC = r"D:\Temp\anomaly_textures\generated"
FOLDERS = {
    "T_I01_": "/Game/MyStuff/Anomalies/I01",
    "T_MagazineAnomaly_": "/Game/MyStuff/Anomalies/Magazine",
    "T_D01_": "/Game/MyStuff/Anomalies/D01",
    "T_F01_": "/Game/MyStuff/Anomalies/F01",
}

tools = unreal.AssetToolsHelpers.get_asset_tools()
done, skipped = [], []
for fname in sorted(os.listdir(SRC)):
    if not fname.endswith(".png"):
        continue
    asset = fname[:-4]
    folder = next((f for k, f in FOLDERS.items() if asset.startswith(k)), None)
    if not folder:
        skipped.append(asset)
        continue
    path = "%s/%s" % (folder, asset)
    existing = unreal.EditorAssetLibrary.does_asset_exist(path)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, fname)
    task.destination_path = folder
    task.destination_name = asset
    task.replace_existing = True
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
    unreal.log("%s %s -> %s" % ("REIMPORTED" if existing else "IMPORTED", fname, path))
    done.append(path)
unreal.log("DONE %d imported, skipped: %s" % (len(done), skipped))
