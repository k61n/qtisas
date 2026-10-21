import os
out = os.path.expanduser("~/benchdata/dan/out")
os.makedirs(out, exist_ok=True)
Dan.Options.setOutputPath(out)
files = Dan.Merge.run("script3-mergingTemplate", output="files")
scriptPrint("R14:", len(files), *sorted(os.path.basename(f) for f in files))
