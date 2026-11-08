"""Stage only the public Orbit assets for Cloudflare deployment."""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[1]
destination = root / "build" / "orbit-web"
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir(parents=True)
for name in ("index.html", "orbit.css", "orbit.js", "flow.js", "drums.js", "clock.js", "effects.js", "session.js", "recorder-worklet.js"):
    shutil.copy2(root / "prototypes" / "orbit" / name, destination / name)
print(f"Prepared 9 public assets in {destination}")
