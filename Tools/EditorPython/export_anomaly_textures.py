"""Editor-side half of the text-anomaly texture round trip: export the base PNGs.

Run headless (no editor window needed):

    UnrealEditor-Cmd.exe <Loop9.uproject> -EnablePlugins=PythonScriptPlugin
        -ExecutePythonScript="Tools/EditorPython/export_anomaly_textures.py"
        -unattended -nopause -nosplash

Writes every Texture2D under /Game/MyStuff/Anomalies and the magazine /
newspaper / book textures from Deko_MatrixDemo to OUT as 4096x4096 PNGs.
Then: py -3 Tools/make_text_anomaly_textures.py <OUT>
Then: reimport_anomaly_textures.py (same command, other script).
"""
import os
import unreal

OUT = r"D:\Temp\anomaly_textures"
os.makedirs(OUT, exist_ok=True)

registry = unreal.AssetRegistryHelpers.get_asset_registry()
paths = []
for folder in ["/Game/MyStuff/Anomalies", "/Game/Deko_MatrixDemo/Apartment/Textures"]:
    for a in registry.get_assets_by_path(folder, recursive=True):
        name = str(a.asset_name)
        cls = str(a.asset_class_path.asset_name)
        if cls != "Texture2D":
            continue
        if folder.endswith("Textures") and not any(k in name for k in ["Magazine", "Newspaper", "Pamphlet", "Books", "Booklet"]):
            continue
        paths.append(str(a.package_name))

unreal.log("exporting %d textures" % len(paths))
for p in paths:
    tex = unreal.load_asset(p)
    if not tex:
        continue
    task = unreal.AssetExportTask()
    task.object = tex
    task.filename = os.path.join(OUT, p.replace("/Game/", "").replace("/", "__") + ".png")
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = unreal.TextureExporterPNG()
    ok = unreal.Exporter.run_asset_export_task(task)
    unreal.log("%s -> %s (%s)" % (p, task.filename, ok))
unreal.log("EXPORT DONE")
