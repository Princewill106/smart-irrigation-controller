# Smart Irrigation Controller (ESP32)

> Waters on soil data and the weather forecast, not on a timer, with a manual override any farmer can use.

![Python](https://img.shields.io/badge/python-3.10%2B-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Offline demo](https://img.shields.io/badge/demo-runs%20offline-brightgreen) ![n8n](https://img.shields.io/badge/n8n-importable%20workflow-EA4B71) ![n8n](https://img.shields.io/badge/n8n-importable%20workflow-EA4B71) ![Series](https://img.shields.io/badge/AI%20Automation-48%2F50-purple)

**Sector:** Agritech & IoT  ·  **Project 48 of 50** in my [AI Automation Projects series](https://github.com/Princewill106/Princewill106)

## The problem

Watering on a fixed schedule wastes water and stresses crops: too much after rain, too little in a dry spell. One sensor tells you about one spot, fields aren't uniform, and any system controlling water needs a way for a human to take charge without tools.

## How it works

```mermaid
flowchart LR
    S0["Soil sensors"]
    S1["ESP32 reads"]
    S2["Threshold + forecast"]
    S3["Relay → valve"]
    S4["Dashboard log"]
    S0 --> S1
    S1 --> S2
    S2 --> S3
    S3 --> S4
```

1. **Sense** — Three capacitive soil-moisture sensors in the root zone are read every 10 minutes; the median is used so one bad sensor can't flood the field.
2. **Decide** — If moisture is below the threshold for the current crop stage and no rain above 5 mm is forecast, irrigation starts.
3. **Act** — A relay drives a 12V solenoid valve. Run time is calculated from the moisture deficit and capped at a hard maximum, with a fuse in line.
4. **Override** — A physical switch forces the valve OFF or ON regardless of the logic.
5. **Log** — Readings and actions are sent over Wi-Fi as JSON to a dashboard or Google Sheet webhook.

## Features

- ESP32 firmware (`irrigation_controller.ino`)
- Median of three sensors for robustness
- Crop-stage thresholds
- Rain-forecast skip
- Hard max run time and manual override
- Python season simulator using the same logic (`main.py`)

## Quick start

```bash
git clone https://github.com/Princewill106/smart-irrigation-controller.git
cd smart-irrigation-controller
python main.py --demo
```

No installs needed: it uses only the Python standard library. The demo runs fully offline using the sample data in this repo.

**Optional: plug in an LLM.** Copy `.env.example` to `.env` and add an Anthropic or OpenAI key. The project will then use the model for the language-heavy steps and fall back to its rule-based logic if a call fails.

Run the tests:

```bash
python -m unittest -v
```

## Sample output

```text
Day  Sensors (%)      Forecast  Rain  Valve  Decision
  0  [43, 44, 46]         0 mm   0 mm  10 min  dry (44% < 45%), watering 10 min
  1  [50, 50, 52]         0 mm   0 mm   0 min  moist enough (50% >= 45%)
  2  [43, 47, 46]         0 mm   0 mm   0 min  moist enough (46% >= 45%)
  3  [39, 38, 41]         0 mm   0 mm  14 min  dry (39% < 45%), watering 14 min
  4  [58, 62, 59]         0 mm  12 mm   0 min  moist enough (59% >= 45%)
  5  [55, 57, 56]         0 mm   0 mm   0 min  moist enough (56% >= 38%)
  6  [69, 68, 93]         0 mm  25 mm   0 min  moist enough (69% >= 38%)
  7  [68, 70, 69]         0 mm  12 mm   0 min  moist enough (69% >= 38%)
  8  [64, 65, 65]         0 mm   0 mm   0 min  moist enough (65% >= 38%)
  9  [59, 60, 58]         0 mm   0 mm   0 min  moist enough (59% >= 38%)
 10  [58, 53, 52]         0 mm   0 mm   0 min  moist enough (53% >= 42%)
 11  [47, 49, 48]         0 mm   0 mm   0 min  manual override OFF
 12  [42, 40, 41]         0 mm   0 mm  10 min  dry (41% < 42%), watering 10 min
 13  [50, 51, 49]         0 mm   0 mm   0 min  moist enough (50% >= 42%)

Total valve time: 34 min over 14 days (a fixed 15 min/day timer would use 210 min)
```

## Use it in n8n (no code needed)

[`n8n-workflow.json`](n8n-workflow.json) is a ready-to-import n8n version of this project, built for n8n 1.x.

1. In n8n, open **Workflows → Import from File** and choose `n8n-workflow.json`.
2. Click **Test workflow**. It runs the full logic on the sample data built into the *Sample data* node, so you can see the results before connecting anything.
3. Open each output node and connect your accounts: Google Sheets (OAuth2), WhatsApp Business Cloud API. For Google Sheets, pick your spreadsheet and sheet.
4. Adjust the settings at the top of the **Logic** node (thresholds, templates, contacts), then **Activate**.

**Flow:** **Webhook** (`POST /webhook/irrigation`) → **Logic** (Code node) → **Route** (Switch)

**Outputs:**
- Tell the farmer (WhatsApp)
- Log watering
- Log reading

The Logic node is a JavaScript port of the rules in `main.py`, so you can run the project in n8n without Python.

## Use it on your own data

Simulate first: `python main.py --demo`. Then flash `irrigation_controller.ino` with the Arduino IDE (ESP32 board package), set your Wi-Fi and webhook URL, calibrate DRY/WET readings for your sensors and wire: sensors to GPIO 32/33/34, relay to GPIO 26, override switch to GPIO 27. Use a fused 12V supply for the valve.

## Project structure

```text
n8n-workflow.json            Importable n8n workflow
.env.example                 Optional API key settings
.gitignore                   Git ignore rules
LICENSE                      MIT license
README.md                    This file
irrigation_controller.ino    Microcontroller firmware (ESP32)
llm.py                       Optional LLM helper (offline fallback)
main.py                      Entry point and core logic
requirements.txt             Dependencies (stdlib only)
test_main.py                 Unit tests
```

## Who this is for

- Smallholder and greenhouse farmers
- School agriculture and robotics clubs
- Agritech students and IoT trainers

## Ideas to extend it

- [ ] Weather API call directly from the ESP32
- [ ] LoRa for fields without Wi-Fi
- [ ] Solar power and deep sleep
- [ ] Per-zone valves

## About

Built by **Ogbu Ugochukwu Princewill**, AI Automation Lead and robotics engineer. I teach people and businesses to build with AI, not just talk about it. [LinkedIn](https://www.linkedin.com/in/ugochukwu-ogbu-653a0816a)

If this helped you, give it a ⭐ and share what you built with it.

## License

MIT — see [LICENSE](LICENSE).
