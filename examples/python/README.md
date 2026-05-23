# Python Examples

Build the native shared library first:

```powershell
cmake --build ..\..\build --target geometer_shared --config Release
```

Install the demo GUI dependency:

```powershell
python -m pip install dearpygui
```

Run the HLR viewer:

```powershell
python hlr_viewer.py ..\..\tests\fixtures\step\embedded_models\SOT-23.STEP
```

Or run a non-GUI projection smoke check:

```powershell
python hlr_viewer.py --project-once ..\..\tests\fixtures\step\embedded_models\SOT-23.STEP
```

The examples import the checkout package from `../../python` when run from this
repository, so they do not need the package installed into the active
environment.
