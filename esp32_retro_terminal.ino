#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

/* --- KERNEL CONFIG --- */
#define LED_PIN         2      // GPIO for LED
#define BUTTON_PIN      0      // GPIO for BOOT button
#define LONG_PRESS_MS   3000   // Hold time for reset
#define AP_SSID         "esp32_tty"
#define AP_PASS         "root1234"

/* --- GLOBALS --- */
Preferences preferences;
WebServer server(80);

String sys_ssid = "";
String sys_pass = "";

unsigned long btn_press_time = 0;
bool led_state = LOW;
bool ap_mode = false;

/* --- UTILS (Kernel Logs) --- */

void klog(const char* module, const char* msg) {
    // Формат: [TIMESTAMP] module: message
    Serial.printf("[%6lu.%03lu] %s: %s\n", millis() / 1000, millis() % 1000, module, msg);
}

/* --- TUI GENERATORS (HTML/CSS) --- */

const char* TTY_CSS = R"rawliteral(
<style>
  :root { --bg: #050505; --fg: #00ff00; --dim: #005500; --inv-bg: #00ff00; --inv-fg: #000000; }

  /* Reset & CRT Base */
  * { box-sizing: border-box; font-family: 'Courier New', 'Lucida Console', monospace; }
  body { 
    background-color: var(--bg); color: var(--fg); 
    padding: 20px; font-size: 14px; margin: 0; 
    overflow-x: hidden; min-height: 100vh;
  }

  /* Scanlines Effect */
  body::before {
    content: " "; display: block; position: absolute;
    top: 0; left: 0; bottom: 0; right: 0;
    background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%);
    background-size: 100% 4px; z-index: 2; pointer-events: none;
  }
  .term-content { position: relative; z-index: 1; max-width: 800px; margin: 0 auto; }

  /* Links as Text Commands */
  a, button { 
    background: transparent; color: var(--fg); border: none; 
    font-size: inherit; text-decoration: none; cursor: pointer; 
    padding: 2px 5px; font-weight: bold;
  }
  a:hover, button:hover { background-color: var(--inv-bg); color: var(--inv-fg); }

  /* Inputs (Transparent) */
  input { 
    background: transparent; border: none; 
    border-bottom: 1px dashed var(--dim); 
    color: var(--fg); font-family: inherit; font-size: inherit; 
    outline: none; width: 200px; margin-left: 10px;
  }
  input:focus { border-bottom: 1px solid var(--fg); }

  /* Utility Classes */
  .prompt { color: #88ff88; font-weight: bold; }
  .dim { color: var(--dim); }
  .bracket { color: #00aa00; }
  .active { background-color: var(--inv-bg); color: var(--inv-fg); }

  /* Blinking Cursor */
  .cursor::after { content: "█"; animation: blink 1s step-end infinite; }
  @keyframes blink { 50% { opacity: 0; } }

  pre { margin: 0; white-space: pre-wrap; word-wrap: break-word; }
</style>
)rawliteral";

String render_page(String content) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ttyS0@esp32</title>";
    html += TTY_CSS;
    html += "</head><body><div class='term-content'>";
    html += content;
    html += "</div></body></html>";
    return html;
}

String get_config_html() {
    int n = WiFi.scanNetworks();
    String net_list = "";

    if (n == 0) {
        net_list = "<span class='dim'>   (no wlan networks found)</span>\n";
    } else {
        for (int i = 0; i < n; i++) {
            net_list += String(WiFi.RSSI(i)) + "dBm  " + WiFi.SSID(i) + "\n";
        }
    }

    String content = R"rawliteral(
<pre>
<span class="prompt">root@esp32:/cfg#</span> iwlist wlan0 scan
<span class="dim">   SIG     SSID</span>
)rawliteral" + net_list + R"rawliteral(

<span class="prompt">root@esp32:/cfg#</span> vi /etc/wpa_supplicant.conf
<span class="dim"># Enter credentials to connect</span>
</pre>
<form action="/save" method="POST" style="margin-top: 10px;">
  <div><span class="dim">ssid=</span><input type="text" name="ssid" placeholder="network_name" required autocomplete="off"></div>
  <div><span class="dim">psk=</span> <input type="password" name="password" placeholder="********" required></div>
  <br>
  <span class="prompt">root@esp32:/cfg#</span> <button type="submit">[ :wq! ]</button> <span class="dim">(write & reboot)</span>
</form>
<span class="cursor"></span>
)rawliteral";
    return render_page(content);
}

String get_status_html() {
    String led_visual = led_state ? "<span class='active'> ON </span>" : "<span class='dim'> OFF </span>";
    String ip_addr = WiFi.localIP().toString();
    uint32_t free_heap = ESP.getFreeHeap();

    String content = R"rawliteral(
<pre>
<span class="prompt">root@esp32:~#</span> htop -b -n 1
MEM_FREE: )rawliteral" + String(free_heap) + R"rawliteral( bytes
UPTIME:   )rawliteral" + String(millis()/1000) + R"rawliteral( sec

<span class="prompt">root@esp32:~#</span> service led status
Status: )rawliteral" + led_visual + R"rawliteral(

<span class="prompt">root@esp32:~#</span> ifconfig wlan0
inet addr: )rawliteral" + ip_addr + R"rawliteral(

<span class="prompt">root@esp32:~#</span> ./control_panel.sh
</pre>
<div style="margin-top: 10px;">
  <span class="bracket">></span> <a href="/toggle">[ TOGGLE_LED ]</a>
</div>
<div style="margin-top: 5px;">
  <span class="bracket">></span> <a href="/">[ REFRESH ]</a>
</div>
<br>
<span class="prompt">root@esp32:~#</span> <span class="cursor"></span>
)rawliteral";
    return render_page(content);
}

String get_save_html(String s, String p) {
    String log = "Writing config: ssid=" + s + "\nRebooting system...";
    String content = R"rawliteral(
<pre>
<span class="prompt">root@esp32:/etc/net#</span> make install
)rawliteral" + log + R"rawliteral(
</pre>
<script>setTimeout(function(){ window.location = '/'; }, 4000);</script>
<span class="cursor"></span>
)rawliteral";
    return render_page(content);
}

/* --- HANDLERS --- */

void handle_root() {
    if (ap_mode) {
        server.send(200, "text/html", get_config_html());
    } else {
        server.send(200, "text/html", get_status_html());
    }
}

void handle_toggle() {
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state);
    klog("gpio", led_state ? "led turned high" : "led turned low");

    // Redirect back to avoid "confirm resubmission" on refresh
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
}

void handle_save() {
    if (server.method() != HTTP_POST) return;

    String n_ssid = server.arg("ssid");
    String n_pass = server.arg("password");

    preferences.begin("wifi", false);
    preferences.putString("ssid", n_ssid);
    preferences.putString("password", n_pass);
    preferences.end();

    server.send(200, "text/html", get_save_html(n_ssid, "******"));

    delay(1000);
    ESP.restart();
}

void start_ap_mode() {
    klog("wifi", "switching to AP mode (monitor)");

    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    klog("net", "interface ap0 up");
    Serial.printf("           addr: %s\n", WiFi.softAPIP().toString().c_str());

    ap_mode = true;

    // Blink sequence to indicate AP mode entry
    for (int i = 0; i < 4; i++) {
        digitalWrite(LED_PIN, HIGH); delay(100);
        digitalWrite(LED_PIN, LOW); delay(100);
    }
}

/* --- MAIN ROUTINES --- */

void setup() {
    Serial.begin(115200);
    delay(500);
    klog("kernel", "boot sequence initiated");

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, led_state);

    preferences.begin("wifi", true);
    sys_ssid = preferences.getString("ssid", "");
    sys_pass = preferences.getString("password", "");
    preferences.end();

    bool force_ap = (digitalRead(BUTTON_PIN) == LOW);

    if (force_ap) {
        klog("interrupt", "manual override (button held)");
        start_ap_mode();
    } else if (sys_ssid == "") {
        klog("config", "no wpa credentials found");
        start_ap_mode();
    } else {
        klog("wifi", "associating with station...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(sys_ssid.c_str(), sys_pass.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println("");

        if (WiFi.status() == WL_CONNECTED) {
            klog("net", "wlan0 link established");
            Serial.printf("           inet addr: %s\n", WiFi.localIP().toString().c_str());
            ap_mode = false;
        } else {
            klog("net", "association timed out");
            start_ap_mode();
        }
    }

    server.on("/", handle_root);
    server.on("/toggle", handle_toggle);
    server.on("/save", handle_save);

    server.onNotFound([](){
        server.send(404, "text/plain", "Error: 404 Segfault (Page not found)");
    });

    server.begin();
    klog("httpd", "server started on port 80");
}

void loop() {
    server.handleClient();

    // Watchdog for button long press (Factory Reset / Config Mode)
    if (!ap_mode) {
        if (digitalRead(BUTTON_PIN) == LOW) {
            if (btn_press_time == 0) {
                btn_press_time = millis();
            } else if (millis() - btn_press_time > LONG_PRESS_MS) {
                klog("interrupt", "long press detected, rebooting to config");
                start_ap_mode();
                // Restart server to serve config page
                server.stop();
                server.begin();
                btn_press_time = 0;
            }
        } else {
            btn_press_time = 0;
        }
    }

    delay(2); // Yield to OS
}
