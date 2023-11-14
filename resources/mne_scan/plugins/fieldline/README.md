FieldLine API README

Prerequisits:
- Python 3.8 needs to be installed - due to an issue in the shared memory library that was fixed
- Download api-example.zip and fieldline_api_XXX.whl

1) Download and extract api-example.zip
2) cd api-example
3) python3.8 -m venv venv
4) . venv/bin/activate
5) pip install -r requirements-api.txt
6) pip install <path to downloaded API whl file>
7) python main.py

Release Notes:
0.0.13 - add SENSOR_READY state
0.0.12 - fix wrong file include
0.0.11 - Add sensor status call
0.0.10 - Few API cleanup items
0.0.2 - add start_adc and stop_adc calls
0.0.1 - initial release
