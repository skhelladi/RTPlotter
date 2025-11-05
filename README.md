# RTPlotter

RTPlotter is a C++/Qt6 application for real-time plotting of tabular data (CSV) produced by CFD codes or any other data producer. The application watches a data file as it grows and updates plots incrementally.

## Key features

- Flexible CSV/text parsing (custom separator, start line, header option, ignore non-numeric lines).
- Parser configuration dialog with live preview.
- Plot configuration dialog: choose which columns are X or Y, graph assignment, style, thickness and color.
- Multiple plots arranged with a `QSplitter` (one `QCustomPlot` per graph).
- Real-time file watching and incremental reading of newly appended lines (via `FileWatcher`).
- Project save/load (`*.rtp`) containing data file path, parser settings and plot configurations.
- Per-CSV sidecar files (`<file>.rtplotter.json`) that store parser + plot settings.
- Export plots to PNG/JPEG/PDF, pause/resume updates, reset zoom.
- SVG icons bundled and rendered to match the current theme (light/dark).

## Project layout

- `CMakeLists.txt` - CMake configuration (Qt6, QCustomPlot, resources).
- `src/`
  - `main.cpp` - application entry point and initialization.
  - `MainWindow.{cpp,h}` - main UI, menus, actions, dialog wiring.
  - `CSVReader.{cpp,h}` - CSV parsing and incremental reads.
  - `PlotManager.{cpp,h}` - manages `QCustomPlot` instances and curves.
  - `FileWatcher.{cpp,h}` - wrapper around `QFileSystemWatcher` plus incremental read logic.
  - `ParserConfigDialog.{cpp,h}` - parser settings UI and preview.
  - `PlotConfigDialog.{cpp,h}` - plot selection/config UI.
  - `qcustomplot.cpp/h` - bundled QCustomPlot library.
- `ui/` - Qt Designer `.ui` files (MainWindow.ui, ParserConfigDialog.ui, PlotConfigDialog.ui).
- `resources/` - SVG icons and `resources.qrc`.
- `scripts/` - packaging helpers: `package-mac.sh`, `package-linux-appimage.sh`.
- `sample.csv` - example CSV for quick testing.

## Important classes (quick contract)

- CSVReader
  - Inputs: file path, separator, start line, header flag, ignore-non-numeric flag
  - Outputs: headers (`QStringList`), data (`QVector<QVector<double>>`), JSON config serialization
  - Errors: returns `false` on parse or IO failure

- PlotManager
  - API: addPlot(id, QCustomPlot*), addCurve(plotId, PlotConfig, x, y), updateCurve(graphId, name, newX, newY)
  - Manages curve objects and efficient replotting

- FileWatcher
  - API: watchFile(path), stop(); emits `fileChanged(const QString&)`
  - Optimized to read only new lines for performance

## User interface summary

- File menu: Open (project or CSV), Save, Save As, Import data (opens parser dialog), Export (image), Exit
- Configuration menu: Plot options (opens plot configuration dialog)
- Help menu: About (shows logo and credits)
- Toolbar: compact buttons for common actions (Open, Save, Pause/Resume, Reset Zoom, Import, Export, Configure)
- Main area: `QSplitter` containing one or more `QCustomPlot` widgets

## Build instructions (macOS / Linux)

Prerequisites:

- Qt6 development packages installed and discoverable by CMake
- CMake (>= 3.16 recommended)
- A C++17-capable compiler (clang on macOS; gcc/clang on Linux)

Build example (from project root):

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -- -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
```

Notes for macOS packaging:

- When built on macOS, the project produces a `.app` bundle (CMake uses `MACOSX_BUNDLE`). To deploy Qt frameworks and plugins into the bundle use `macdeployqt` (provided by Qt). A helper script is provided:

```bash
# usage: ./scripts/package-mac.sh /path/to/macdeployqt
# optionally: export CODESIGN_IDENTITY="Developer ID Application: Your Name"
./scripts/package-mac.sh /path/to/macdeployqt
```

The script produces `build/RTPlotter.dmg`. For public distribution you should codesign and notarize the bundle (the script contains guidance and optional automated steps).

Notes for Linux/AppImage:

- `scripts/package-linux-appimage.sh` prepares an AppDir and can produce an AppImage. Tools such as `linuxdeployqt` or `appimagetool` may be required; inspect the script for details.

## Run the app

- macOS (from `build`):

```bash
./RTPlotter.app/Contents/MacOS/RTPlotter
```

- Linux (when a plain executable is built):

```bash
./RTPlotter
```

Running from a terminal is recommended during development so you can see `qDebug()` output.

## Troubleshooting

- Invisible file dialogs on macOS:
  - The code now forces `QFileDialog::DontUseNativeDialog` for critical file dialogs to avoid native dialogs being hidden or blocked. If you still have issues, try running from a terminal and observe `qDebug()` logs.
  - Optionally, set the global attribute in `main.cpp` before creating the application:

    ```cpp
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    ```

- Code signing and notarization (macOS):
  - If packaging fails because an identity is missing, list available identities with:

    ```bash
    security find-identity -p codesigning -v
    ```

  - If you need to import a `.p12` that contains your private key, import it into the login keychain (example):

    ```bash
    security import /path/to/cert.p12 -k ~/Library/Keychains/login.keychain-db -P "p12-password" -T /usr/bin/codesign
    ```

  - The packaging script can remove an incomplete signature left by `macdeployqt` for local testing. For distribution sign with your Developer ID and notarize the DMG.

- Crash while running `ui->setupUi(this)`:
  - Perform a clean build: delete `build/`, re-run CMake and rebuild. Stale generated files (moc/uic) can cause runtime crashes.

- Missing Qt plugins after packaging:
  - Run the binary with plugin debug enabled to see what fails:

    ```bash
    QT_DEBUG_PLUGINS=1 ./RTPlotter.app/Contents/MacOS/RTPlotter
    ```

  - Verify that `RTPlotter.app/Contents/PlugIns` contains `platforms/libqcocoa.dylib` (mac) or the appropriate platform plugin on Linux.

## Examples and tests

- `sample.csv` is included to quickly test import and plot configuration via `File -> Import data...`.

## Contributing

Contributions welcome. Follow the CMake layout and add unit tests for parsing (`CSVReader`) where appropriate.

## Contact

Author: Prof. Sofiane KHELLADI <sofiane@khelladi.page>
