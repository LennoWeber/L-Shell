# L-Shell UI Design-Plan 2026
## Kompletter Reset & Neudesign für Fedora + Hyprland

---

## 📋 ZIELE

### Primärziele
- **Kompletter Reset**: Alle bestehenden UI-Entwürfe und Desktops entfernen
- **Modernstes Design**: Extrem modern, professionell, klassig aussehend
- **Vollständige Funktionalität**: Alle notwendigen Funktionen erhalten
- **Hyprland-Optimierung**: Perfekt für Fedora mit Hyprland Wayland-Compositor
- **Unabhängiger Plan**: Zuerst Vision erstellen, dann umsetzen

### Design-Prinzipien
1. **Minimalistisch**: Weniger ist mehr - klare Linien, keine Überladung
2. **Professionell**: Business-ready, keine Spielereien
3. **Modern**: Aktuelle Design-Trends (2026) + zeitlose Eleganz
4. **Konsistent**: Einheitliches Design-System durch alle Komponenten
5. **Funktional**: Jedes Element hat einen Zweck
6. **Barrierefrei**: Gute Lesbarkeit, Kontraste, Skalierbarkeit

---

## 🎨 FARBPALLETTE

### Primärfarben
- **Hauptfarbe**: `#1a1a2e` (Dunkelblau-Schwarz) - Professional & Modern
- **Akzentfarbe**: `#16213e` (Tiefes Blau) - Für Highlights
- **Sekundärfarbe**: `#0f3460` (Königsblau) - Für Aktionen

### Neutralfarben
- **Hintergrund**: `#0a0a14` (Fast Schwarz) - Für maximale Fokussierung
- **Oberfläche**: `#161b26` (Dunkelgrau) - Für Panels
- **Text Primär**: `#e8e8e8` (Hellgrau) - Haupttext
- **Text Sekundär**: `#a0a0a0` (Grau) - Hilfstext
- **Text Deaktiviert**: `#6c6c6c` (Dunkelgrau)

### Statusfarben
- **Erfolg**: `#4caf50` (Grün)
- **Warnung**: `#ff9800` (Orange)
- **Fehler**: `#f44336` (Rot)
- **Information**: `#2196f3` (Blau)

### Transparenz & Effekte
- **Panel Transparenz**: 85% Opazität mit leichter Unschärfe
- **Dropdown Transparenz**: 90% Opazität
- **Shadow Effekt**: Subtile Drop-Shadows für Tiefe

---

## 📐 TYPOGRAPHIE

### Schriftarten
- **Primär**: `Inter` (Moderne, klare Sans-Serif)
- **Sekundär**: `JetBrains Mono` (Für Code/Terminal-Anzeigen)
- **Fallback**: `Noto Sans` (Für vollständige Unicode-Unterstützung)

### Schriftgrößen
- **Tiny**: 10px (Hilfstexte, Labels)
- **Small**: 12px (Widget-Texte)
- **Base**: 14px (Standardtext)
- **Large**: 16px (Überschriften in Panels)
- **X-Large**: 20px (Hauptüberschriften)
- **XX-Large**: 24px (Titel in Control Center)

### Schriftgewichte
- **Regular**: 400 (Standard)
- **Medium**: 500 (Betonte Texte)
- **SemiBold**: 600 (Überschriften)
- **Bold**: 700 (Wichtige Überschriften)

---

## 🎯 ABSTÄNDE & LAYOUT

### Grid-System
- **Basis-Grid**: 8px
- **Komponenten-Grid**: 16px
- **Abschnitts-Grid**: 24px

### Abstände
- **XS**: 4px (Minimaler Abstand)
- **SM**: 8px (Kleiner Abstand)
- **MD**: 16px (Standardabstand)
- **LG**: 24px (Großer Abstand)
- **XL**: 32px (Sehr großer Abstand)

### Rundungen
- **Keine**: 0px (Für technische/minimalistische Elemente)
- **Klein**: 4px (Für Subtile Elemente)
- **Standard**: 8px (Für die meisten Komponenten)
- **Groß**: 12px (Für Panels und Container)
- **Full**: 9999px (Für Kreise/Pills)

---

## 🖥️ LEISTEN DESIGN (Bar)

### Positionierung
- **Standard**: Unten (Hyprland-typisch)
- **Alternative**: Oben oder Links/Rechts (konfigurierbar)
- **Breite**: 48px (schmal) bis 64px (standard)

### Struktur
```
┌─────────────────────────────────────────────────────────────┐
│  [Start]  [App-Launcher]  [Fenster]  [Widgets]  [System]  [Uhr] │
└─────────────────────────────────────────────────────────────┘
```

### Bereiche
1. **Start-Bereich** (Links)
   - Logo/Icon (24x24px)
   - Start-Button (optional)
   
2. **App-Launcher-Bereich**
   - Suchfeld (kann ausgeklappt werden)
   - Schnellstarter-Icons
   
3. **Fenster-Bereich** (Zentrum)
   - Aktive Fenster als Tabs
   - Workspace-Indikatoren
   
4. **Widgets-Bereich**
   - Dynamische Widgets (Uhr, System, etc.)
   
5. **System-Bereich** (Rechts)
   - System-Tray
   - Benachrichtigungen
   - Control Center-Button
   - Power-Menü

### Visuelles Design
- **Hintergrund**: Halbdurchsichtig mit Unschärfe-Effekt
- **Rahmen**: Subtile Border (1px, 20% Opazität)
- **Hover-Effekt**: Leichte Aufhellung (5%)
- **Aktiver Tab**: Akzentfarbe-Unterstrich (2px)

---

## 🎛️ CONTROL CENTER DESIGN

### Gesamtlayout
```
┌─────────────────────────────────────────────┐
│  [Suchleiste]                              [✕] │
├─────────────────────────────────────────────┤
│                                                 │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐        │
│  │  WiFi   │ │  Sound  │ │  Power  │        │
│  │  ⬆80%  │ │  🔊50%  │ │  🔋95%  │        │
│  └─────────┘ └─────────┘ └─────────┘        │
│                                                 │
│  ┌───────────────────────────────────────┐  │
│  │           Quick Settings                │  │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐    │  │
│  │  │WiFi │ │Vol+ │ │Vol- │ │🔦│    │  │
│  │  └─────┘ └─────┘ └─────┘ └─────┘    │  │
│  └───────────────────────────────────────┘  │
│                                                 │
│  ┌───────────────────────────────────────┐  │
│  │  ✏️  System-Einstellungen               │  │
│  │  ⚙️  L-Shell-Einstellungen             │  │
│  │  🔒  Sperren                             │  │
│  │  ⏻  Suspend                              │  │
│  │  ⏾  Neustart                             │  │
│  │  ⏻  Herunterfahren                       │  │
│  └───────────────────────────────────────┘  │
│                                                 │
└─────────────────────────────────────────────┘
```

### Tabs
1. **Home** - Schnellzugriff auf häufige Einstellungen
2. **WiFi** - Netzwerkverbindungen
3. **Sound** - Audioeinstellungen
4. **Bluetooth** - Geräteverwaltung
5. **Power** - Energieeinstellungen
6. **Display** - Monitor-Konfiguration
7. **System** - Systeminformationen
8. **Benachrichtigungen** - Benachrichtigungscenter

### Visuelles Design
- **Größe**: 400px Breite, 600px Höhe (skalierbar)
- **Position**: Rechtsbündig oder zentriert
- **Hintergrund**: 90% Opazität, leichte Unschärfe
- **Animation**: Slide-in von rechts, Fade-in
- **Schließbutton**: ✕ oben rechts

---

## 🧩 WIDGETS DESIGN

### Allgemeine Widget-Prinzipien
- **Größe**: 24px Höhe (in der Leiste)
- **Abstand**: 8px zwischen Widgets
- **Hover**: Leichte Skalierung (1.05x) + Hintergrundaufhellung
- **Klick**: Subtiler Druckeffekt

### Einzelne Widgets

#### 1. Uhr-Widget
```
┌──────────────┐
│  14:30:45    │  ← Standard (Text)
│  🕘 14:30    │  ← Mit Icon
└──────────────┘
```
- **Format**: 24h oder 12h (konfigurierbar)
- **Datum**: Bei Hover angezeigt
- **Icon**: Optionales Uhr-Icon

#### 2. Datum-Widget
```
┌──────────────┐
│  Do, 15. Okt │
│  2026        │
└──────────────┘
```
- **Format**: Kurzes oder langes Datum
- **Wochentag**: Abgekürzt oder ausführlich

#### 3. Batterie-Widget
```
┌─────────┐
│  🔋 95% │  ← Normal
│  ⚡ 25% │  ← Ladezustand
│  🪫 5%  │  ← Kritisch
└─────────┘
```
- **Icon**: Dynamisch nach Ladezustand
- **Farben**: Grün > Gelb > Rot

#### 4. Netzwerk-Widget
```
┌─────────┐
│  📶 80% │  ← WLAN
│  🌐     │  ← Ethernet
│  ❌     │  ← Keine Verbindung
└─────────┘
```
- **Stärke**: Signalstärke-Indikator
- **Typ**: Unterschiedliche Icons für WLAN/Ethernet

#### 5. Lautstärke-Widget
```
┌─────────┐
│  🔊 50% │
│  🔇     │  ← Stumm
└─────────┘
```
- **Slider**: Bei Klick/Hover erscheint Mini-Slider
- **Visualisierung**: Optionale Soundwellen-Animation

#### 6. Workspace-Widget
```
┌─────────────┐
│  1 2 [3] 4  │  ← Aktiver Workspace hervorgehoben
└─────────────┘
```
- **Stil**: Minimalistische Zahlen oder Symbole
- **Aktiv**: Akzentfarbe-Hintergrund

#### 7. Fenster-Widget (Taskbar)
```
┌─────────────────┐
│  [Firefox] [VSCode] [Terminal] │
└─────────────────┘
```
- **Aktives Fenster**: Unterstrich in Akzentfarbe
- **Hover**: Leichte Hervorhebung
- **Gruppierung**: Fenster desselben Apps können gruppiert werden

#### 8. System-Monitor-Widget
```
┌──────────────┐
│  CPU: 45%    │
│  RAM: 12GB   │
│  GPU: 60°C   │
└──────────────┘
```
- **Anzeige**: CPU, RAM, GPU, Temperatur
- **Farben**: Dynamisch nach Auslastung

#### 9. Wetter-Widget
```
┌──────────────┐
│  ☀️ 22°C     │  ← Aktuelles Wetter
│  ⛅ 18-25°C   │  ← Vorhersage
└──────────────┘
```
- **Icon**: Dynamisch nach Wetterlage
- **Information**: Temperatur + Bedingung

#### 10. Benachrichtigungs-Widget
```
┌─────────┐
│  🔔 3   │  ← Ungelesene Benachrichtigungen
│  🔔     │  ← Keine Benachrichtigungen
└─────────┘
```
- **Zähler**: Anzahl ungelesener Benachrichtigungen
- **Popup**: Bei Klick erscheint Benachrichtigungscenter

#### 11. Kalender-Widget
```
┌──────────────┐
│  📅 15       │  ← Tag im Monat
│  Do           │  ← Wochentag
└──────────────┘
```
- **Anzeige**: Aktueller Tag
- **Popup**: Bei Klick voller Kalender

#### 12. Medien-Widget
```
┌────────────────┐
│  ▶️  Spotify    │  ← Wird abgespielt
│  ⏸️  Pause     │  ← Pausiert
│  ✕  Kein Titel  │  ← Nichts läuft
└────────────────┘
```
- **Steuerung**: Play/Pause, Vor/Zurück
- **Information**: Titel + Künstler

#### 13. Bluetooth-Widget
```
┌─────────┐
│  🔵 3   │  ← Verbundene Geräte
│  🔵 ❌  │  ← Deaktiviert
└─────────┘
```
- **Status**: Aktiv/Inaktiv
- **Geräte**: Anzahl verbundener Geräte

#### 14. Helligkeit-Widget
```
┌─────────┐
│  🌞 75% │
└─────────┘
```
- **Slider**: Bei Klick/Hover erscheint Mini-Slider
- **Icon**: Sonne oder Mond (je nach Tageszeit)

#### 15. Tastatur-Layout-Widget
```
┌─────────┐
│  DE     │  ← Aktuelles Layout
│  US     │  ← Alternative Layouts
└─────────┘
```
- **Anzeige**: Aktuelles Tastaturlayout
- **Wechsel**: Bei Klick Layout wechseln

#### 16. Nightlight-Widget
```
┌─────────┐
│  🌙 ON  │  ← Aktiviert
│  🌞 OFF │  ← Deaktiviert
└─────────┘
```
- **Status**: Ein/Aus
- **Intensität**: Bei Klick/Hover Slider

#### 17. Screenshot-Widget
```
┌─────────┐
│  📷     │
└─────────┘
```
- **Funktion**: Schnellzugriff auf Screenshot-Funktionen

#### 18. Power-Profile-Widget
```
┌──────────────┐
│  ⚡ Performance │
│  🌿 Balanced  │
│  🔋 Power Save│
└──────────────┘
```
- **Anzeige**: Aktuelles Profil
- **Wechsel**: Bei Klick Profil wechseln

#### 19. Clipboard-Widget
```
┌─────────┐
│  📋 5   │  ← Anzahl gespeicherter Einträge
└─────────┘
```
- **Funktion**: Schnellzugriff auf Clipboard-History

#### 20. Privatsphäre-Widget
```
┌─────────┐
│  🔒     │  ← Mikrofon/Kamera gesperrt
│  🔓     │  ← Entsperrt
└─────────┘
```
- **Status**: Sperrstatus von Mikrofon/Kamera

#### 21. Tray-Widget
```
┌──────────────┐
│  [App1] [App2] [App3] │
└──────────────┘
```
- **Funktion**: System-Tray Icons
- **Anordnung**: Dynamisch basierend auf verfügbaren Apps

#### 22. Spacer-Widget
```
┌─────────┐
│         │  ← Leerer Platzhalter
└─────────┘
```
- **Funktion**: Flexibler Abstandshalter
- **Größe**: Konfigurierbar

---

## 🏗️ CONTROL CENTER TABS - DETAILLIERT

### Home-Tab
```
┌─────────────────────────────────────────────┐
│  🔍 Suche...                               [✕] │
├─────────────────────────────────────────────┤
│                                                 │
│  Schnellzugriff                            │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐       │
│  │WiFi │ │Vol+ │ │🔦│ │🔇│       │
│  │80%  │ │75%  │ │ON   │ │OFF  │       │
│  └─────┘ └─────┘ └─────┘ └─────┘       │
│                                                 │
│  Systemstatus                              │
│  ┌─────────────────────────────────────┐   │
│  │  CPU: 45%  │  RAM: 12GB  │  GPU: 60°C │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Benachrichtigungen                        │
│  ┌─────────────────────────────────────┐   │
│  │  Keine neuen Benachrichtigungen       │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### WiFi-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                    WiFi              │
├─────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  📶  Aktuelles Netzwerk               │   │
│  │  Name: MyWiFi                          │   │
│  │  Signal: 80%                           │   │
│  │  IP: 192.168.1.100                     │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Verfügbare Netzwerke:                     │
│  ┌─────────────────────────────────────┐   │
│  │  📶 MyWiFi            ████ 80%  ✓    │   │
│  │  📶 GuestWiFi         ████ 60%       │   │
│  │  📶 NeighborWiFi      ███  40%  🔒    │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  🔄 Netzwerk aktualisieren             │   │
│  │  ⚙️  Netzwerkeinstellungen             │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### Sound-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                    Sound            │
├─────────────────────────────────────────────┤
│                                                 │
│  Ausgabe:                                      │
│  ┌─────────────────────────────────────┐   │
│  │  🔊 Lautsprecher (Built-in)          │   │
│  │  ┌─────────────────────────────────┐ │   │
│  │  │  ████████░░░░░░░░░░░░  50%          │ │   │
│  │  └─────────────────────────────────┘ │   │
│  │  [Mute]                                │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Eingabe:                                      │
│  ┌─────────────────────────────────────┐   │
│  │  🎤 Mikrofon (Built-in)              │   │
│  │  ┌─────────────────────────────────┐ │   │
│  │  │  ████████████████░░░░  75%          │ │   │
│  │  └─────────────────────────────────┘ │   │
│  │  [Mute]                                │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Geräte:                                      │
│  ┌─────────────────────────────────────┐   │
│  │  🎧 Kopfhörer                         │   │
│  │  🔊 Bluetooth-Lautsprecher            │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### Bluetooth-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                  Bluetooth        │
├─────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  🔵 Bluetooth                          │   │
│  │  Status: Eingeschaltet                 │   │
│  │  Sichtbarkeit: Sichtbar                │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Verbundene Geräte:                         │
│  ┌─────────────────────────────────────┐   │
│  │  🎧 AirPods Pro         ✓ Verbunden   │   │
│  │  🖱️  Magic Mouse        ✓ Verbunden   │   │
│  │  ⌨️  Magic Keyboard     ❌ Getrennt   │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Verfügbare Geräte:                         │
│  ┌─────────────────────────────────────┐   │
│  │  📱  iPhone 15          [Verbinden]    │   │
│  │  🎧 Sony WH-1000XM5     [Verbinden]    │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  + Gerät hinzufügen                    │   │
│  │  ⚙️  Bluetooth-Einstellungen           │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### Power-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                    Power             │
├─────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  🔋 Batterie                           │   │
│  │  Ladezustand: 95%                       │   │
│  │  Status: Lädt                          │   │
│  │  Verbleibend: ~8 Stunden               │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Energieprofil:                              │
│  ┌─────────────────────────────────────┐   │
│  │  ⚡ Performance                       ✓ │   │
│  │  🌿 Balanced                           │   │
│  │  🔋 Power Save                         │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Schnellaktionen:                             │
│  ┌─────────────────────────────────────┐   │
│  │  ⏻ Suspend              [Jetzt]        │   │
│  │  ⏾ Neustart             [Jetzt]        │   │
│  │  ⏻ Herunterfahren       [Jetzt]        │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### Display-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                   Display          │
├─────────────────────────────────────────────┤
│                                                 │
│  Monitore:                                     │
│  ┌─────────────────────────────────────┐   │
│  │  🖥️  Primärer Monitor (1920x1080)     │   │
│  │     [100%]  [Skalierung: 1.0]        │   │
│  │  ┌─────────┐                           │   │
│  │  │         │                           │   │
│  │  │  ← Visualisierung                  │   │
│  │  │         │                           │   │
│  │  └─────────┘                           │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Anordnung:                                   │
│  ┌─────────────────────────────────────┐   │
│  │  [ 🖥️ 1 ] [ 🖥️ 2 ] [ 🖥️ 3 ]         │   │
│  │  ← Grafische Anordnung                 │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Einstellungen:                               │
│  ┌─────────────────────────────────────┐   │
│  │  🌓 Helligkeit: 75%                    │   │
│  │  🌙 Nightlight: ON                     │   │
│  │  🎯 HDR: OFF                           │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### System-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück                    System          │
├─────────────────────────────────────────────┤
│                                                 │
│  Systeminformationen:                          │
│  ┌─────────────────────────────────────┐   │
│  │  🖥️  System: Fedora Linux 40          │   │
│  │  🏗️  Kernel: 6.8.10-300.fc40          │   │
│  │  💾 Uptime: 2 Tage, 5 Stunden           │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Hardware:                                     │
│  ┌─────────────────────────────────────┐   │
│  │  CPU: AMD Ryzen 9 7950X (16C/32T)     │   │
│  │  GPU: AMD Radeon RX 7900 XTX          │   │
│  │  RAM: 64GB DDR5                        │   │
│  │  Storage: 2TB NVMe                    │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Performance:                                 │
│  ┌─────────────────────────────────────┐   │
│  │  CPU: ████████░░░░░░░░ 45%             │   │
│  │  RAM: ████████████░░░░ 70%             │   │
│  │  GPU: ████████████████ 80%             │   │
│  │  Disk: ████░░░░░░░░░░ 25%             │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Aktionen:                                     │
│  ┌─────────────────────────────────────┐   │
│  │  🔄 System aktualisieren              │   │
│  │  🛠️  Systemeinstellungen öffnen      │   │
│  │  🔍 Logs anzeigen                      │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

### Notifications-Tab
```
┌─────────────────────────────────────────────┐
│  ← Zurück               Benachrichtigungen    │
├─────────────────────────────────────────────┤
│                                                 │
│  Letzte Benachrichtigungen:                    │
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  🔔 Heute                              │   │
│  │  ┌───────────────────────────────┐   │   │
│  │  │  📧 Email - Neue Nachricht        │   │   │
│  │  │  Von: user@example.com           │   │   │
│  │  │  Vor 2 Stunden                     │   │   │
│  │  │  [Lesen] [Archivieren]            │   │   │
│  │  ├───────────────────────────────┤   │   │
│  │  │  💬 Discord - Neue Nachricht      │   │   │
│  │  │  Von: Freund                       │   │   │
│  │  │  Vor 1 Stunde                      │   │   │
│  │  │  [Lesen] [Antworten]              │   │   │
│  │  └───────────────────────────────┘   │   │
│  │                                                 │
│  │  📅 Gestern                            │   │
│  │  ┌───────────────────────────────┐   │   │
│  │  │  🔄 System - Update verfügbar    │   │   │
│  │  │  24 Pakete können aktualisiert    │   │   │
│  │  │  werden                            │   │   │
│  │  │  Vor 1 Tag                         │   │   │
│  │  │  [Installieren] [Später]           │   │   │
│  │  └───────────────────────────────┘   │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  Einstellungen:                               │
│  ┌─────────────────────────────────────┐   │
│  │  🔔 Benachrichtigungen aktivieren    ✓ │   │
│  │  🕐 Nicht stören: OFF                │   │
│  │  🎵 Benachrichtigungston: ON         ✓ │   │
│  └─────────────────────────────────────┘   │
│                                                 │
│  ┌─────────────────────────────────────┐   │
│  │  🗑️  Alle als gelesen markieren        │   │
│  │  ⚙️  Benachrichtigungseinstellungen   │   │
│  └─────────────────────────────────────┘   │
│                                                 │
└─────────────────────────────────────────────┘
```

---

## 🎨 DESIGN-PATTERNS & KOMPONENTEN

### Buttons
```
┌─────────┐  ┌─────────┐  ┌─────────┐
│  Primär  │  │ Sekundär │  │  Tertiar │
└─────────┘  └─────────┘  └─────────┘
```

**Primär-Button:**
- Hintergrund: Akzentfarbe (`#0f3460`)
- Text: Weiß (`#ffffff`)
- Hover: Aufhellung (10%)
- Active: Abdunkelung (10%)
- Rundung: 8px
- Padding: 12px 24px

**Sekundär-Button:**
- Hintergrund: Transparent
- Border: 1px, 50% Opazität
- Text: Primärtext
- Hover: Hintergrund 10% Opazität
- Active: Hintergrund 20% Opazität
- Rundung: 8px
- Padding: 12px 24px

**Tertiar-Button (Icon-Only):**
- Hintergrund: Transparent
- Text: Primärtext
- Hover: Hintergrund 10% Opazität
- Active: Hintergrund 20% Opazität
- Größe: 32px x 32px
- Rundung: 8px

### Input-Felder
```
┌─────────────────┐
│  🔍 Suche...    │
└─────────────────┘
```
- Hintergrund: Oberflächenfarbe (`#161b26`)
- Border: 1px, 20% Opazität
- Text: Primärtext
- Placeholder: Sekundärtext, 50% Opazität
- Focus: Border in Akzentfarbe, 100% Opazität
- Rundung: 8px
- Padding: 12px 16px

### Slider
```
┌─────────────────────────────┐
│  ████████░░░░░░░░░░░░  50%  │
│  ◉                       ◉ │
└─────────────────────────────┘
```
- Track: 4px Höhe, 30% Opazität
- Fill: Akzentfarbe, 100% Opazität
- Thumb: 16px x 16px, Akzentfarbe
- Hover: Thumb skaliert auf 18px
- Active: Thumb skaliert auf 20px
- Rundung: 2px (Track), 50% (Thumb)

### Switch
```
┌───────────┐
│  ◉ ON     │
│  ◎ OFF    │
└───────────┘
```
- Track: 40px Breite, 20px Höhe
- Thumb: 16px x 16px
- ON: Thumb bei 80% (rechts), Akzentfarbe Hintergrund
- OFF: Thumb bei 20% (links), Grauer Hintergrund
- Animation: 200ms Slide + Fade
- Rundung: 10px (Track), 50% (Thumb)

### Checkbox
```
┌─────┐
│  ✓  │  ← Aktiviert
│     │  ← Deaktiviert
└─────┘
```
- Größe: 20px x 20px
- Border: 1px, 50% Opazität
- Aktiviert: Akzentfarbe Hintergrund + ✓ Icon
- Hover: Border 100% Opazität
- Active: Hintergrund 10% Opazität
- Rundung: 4px

### Radio Button
```
┌─────┐
│  ◉  │  ← Aktiviert
│  ◯  │  ← Deaktiviert
└─────┘
```
- Größe: 20px x 20px
- Border: 1px, 50% Opazität
- Aktiviert: Akzentfarbe Hintergrund + ◉ Punkt
- Hover: Border 100% Opazität
- Active: Hintergrund 10% Opazität
- Rundung: 50%

### Cards
```
┌─────────────────────┐
│  Titel               │
│  Inhalt              │
│                     │
└─────────────────────┘
```
- Hintergrund: Oberflächenfarbe (`#161b26`)
- Border: 1px, 10% Opazität
- Padding: 16px
- Rundung: 12px
- Shadow: 0 4px 8px rgba(0,0,0,0.1)

### Tooltips
```
┌──────────────┐
│  Tooltip-Text│
└──────────────┘
```
- Hintergrund: Hauptfarbe (`#1a1a2e`)
- Text: Primärtext
- Padding: 8px 12px
- Rundung: 6px
- Shadow: 0 4px 12px rgba(0,0,0,0.3)
- Animation: Fade-in 150ms

### Context Menus
```
┌─────────────────┐
│  Option 1       │
│  Option 2       │
│  Option 3       │
└─────────────────┘
```
- Hintergrund: Oberflächenfarbe (`#161b26`)
- Text: Primärtext
- Hover: Hintergrund 15% Opazität
- Selected: Hintergrund 25% Opazität
- Padding: 8px 12px
- Rundung: 6px
- Shadow: 0 8px 16px rgba(0,0,0,0.2)
- Animation: Slide + Fade 150ms

### Modal Dialogs
```
┌─────────────────────────────┐
│  Titel                     [✕] │
├─────────────────────────────┤
│  Inhalt                        │
├─────────────────────────────┤
│  [Abbrechen]          [OK]    │
└─────────────────────────────┘
```
- Hintergrund: Oberflächenfarbe (`#161b26`)
- Border: 1px, 20% Opazität
- Padding: 24px
- Rundung: 12px
- Shadow: 0 16px 32px rgba(0,0,0,0.3)
- Overlay: 50% dunkler Hintergrund
- Animation: Fade + Scale 200ms

---

## 🎭 ANIMATIONEN & ÜBERGÄNGE

### Timing-Funktionen
- **Ease Out**: Für Elemente die erscheinen (200ms)
- **Ease In Out**: Für Interaktionen (150ms)
- **Ease In**: Für Elemente die verschwinden (150ms)
- **Spring**: Für elastische Effekte (300ms)

### Animationstypen
1. **Fade**: 150ms, ease-out
   - Elemente die ein-/ausgeblendet werden
2. **Slide**: 200ms, ease-out
   - Panels die ein-/ausfahren
3. **Scale**: 150ms, ease-out
   - Buttons beim Hover
4. **Pulse**: 1000ms, ease-in-out
   - Ladeindikatoren
5. **Shake**: 300ms, spring
   - Fehlerzustände

### Spezifische Animationen
- **Control Center**: Slide-in von rechts (300ms)
- **Dropdowns**: Fade + Slide von oben (200ms)
- **Tooltips**: Fade (150ms), Delay 300ms
- **Buttons**: Scale (1.05x) beim Hover (100ms)
- **Switch/Toggle**: Slide + Fade (200ms)
- **Tab-Wechsel**: Fade (150ms)
- **Fenster-Fokus**: Border-Animation (200ms)

---

## 🎨 VISUELLE EFFEKTE

### Transparenz & Unschärfe
- **Panel-Hintergründe**: 85-90% Opazität
- **Unschärfe-Effekt**: 8px Radius (Gaussian Blur)
- **Farbüberlagerung**: Leichte Farbtönung für Konsistenz

### Schatten
- **Kleine Schatten**: 0 2px 4px rgba(0,0,0,0.1)
- **Mittlere Schatten**: 0 4px 8px rgba(0,0,0,0.15)
- **Große Schatten**: 0 8px 16px rgba(0,0,0,0.2)
- **XL Schatten**: 0 16px 32px rgba(0,0,0,0.25)

### Glow-Effekte
- **Focus-Indikator**: 0 0 0 2px Akzentfarbe (20% Opazität)
- **Hover-Effekt**: 0 0 8px 0 Akzentfarbe (10% Opazität)
- **Aktiver Zustand**: 0 0 16px 0 Akzentfarbe (20% Opazität)

### Farbverläufe
- **Primärer Gradient**: Linear von `#0f3460` zu `#16213e`
- **Sekundärer Gradient**: Linear von `#1a1a2e` zu `#161b26`
- **Status-Gradient**: Linear von Grün zu Gelb zu Rot

---

## 🖼️ HINTERGRÜNDE & WALLPAPER

### Wallpaper-Stil
- **Dunkelmodus**:Standard (perfekt für unser Farbschema)
- **Hintergrundfarbe**: `#0a0a14` (fast schwarz)
- **Akzente**: Subtile Muster oder Texturen (optional)
- **Dynamisch**: Tageszeit-basierte Anpassung

### Desktop-Elemente
- **Icons**: Monochrom oder mit Akzentfarbe
- **Text**: Hoher Kontrast für Lesbarkeit
- **Fenster**: Abgerundete Ecken (12px)
- **Schatten**: Subtile Fenster-Schatten

---

## 🎯 RESPONSIVE DESIGN

### Breakpoints
- **XS**: < 800px (Mobile-ähnlich)
- **SM**: 800px - 1200px (Tablet)
- **MD**: 1200px - 1600px (Laptop)
- **LG**: 1600px - 2000px (Desktop)
- **XL**: > 2000px (Ultra-Wide)

### Anpassungen
- **Leisten**: Höhe reduziert auf 40px (XS/SM), 48px (MD), 56px (LG/XL)
- **Control Center**: Breite angepasst (320px XS, 400px SM/MD, 500px LG/XL)
- **Widgets**: Größere Touch-Ziele auf mobilen Geräten
- **Text**: Schriftgrößen skalieren

---

## 🎨 THEMING & ANPASSUNG

### Thema-Varianten
1. **Dunkel** (Standard)
   - Hintergrund: `#0a0a14`
   - Oberflächen: `#161b26`
   - Akzent: `#0f3460`

2. **Hell**
   - Hintergrund: `#f5f5f5`
   - Oberflächen: `#ffffff`
   - Akzent: `#1976d2`

3. **System**
   - Follows system theme (light/dark)

### Benutzerdefinierte Themen
- **Farben**: Alle Primär-, Sekundär- und Akzentfarben anpassbar
- **Schriftarten**: Alle Schriftarten ersetzbar
- **Größen**: Alle Abmessungen anpassbar
- **Effekte**: Transparenz, Unschärfe, Schatten anpassbar

---

## 🔧 TECHNISCHE ANFORDERUNGEN

### Performance
- **Rendering**: GPU-beschleunigt (Hyprland)
- **FPS**: Mindestens 60 FPS auf allen Geräten
- **Memory**: < 100MB RAM-Verbrauch
- **CPU**: < 5% CPU-Auslastung im Leerlauf

### Kompatibilität
- **Wayland**: Vollständige Unterstützung
- **X11**: Fallback-Unterstützung
- **Hyprland**: Optimierte Integration
- **GNOME/KDE**: Kompatibilitätsmodus

### Barrierefreiheit
- **Kontraste**: WCAG 2.1 AA Kompatibel
- **Skalierung**: 100% - 200% Unterstützung
- **Tastatur**: Vollständige Tastaturnavigation
- **Screen Reader**: Unterstützung für Orca/NVDA

---

## 📁 DATEISTRUKTUR (Neu)

```
L-Shell/
├── src/
│   ├── shell/
│   │   ├── bar/
│   │   │   ├── bar.cpp/.h
│   │   │   ├── bar_instance.h
│   │   │   ├── bar_design.cpp/.h      # NEU
│   │   │   └── widgets/
│   │   │       ├── widget.cpp/.h
│   │   │       ├── widget_design.cpp/.h  # NEU
│   │   │       └── [alle widget-dateien]
│   │   ├── control_center/
│   │   │   ├── control_center_panel.cpp/.h
│   │   │   ├── control_center_design.cpp/.h  # NEU
│   │   │   └── tabs/
│   │   │       └── [alle tab-dateien]
│   │   └── design/
│   │       ├── colors.cpp/.h         # NEU
│   │       ├── typography.cpp/.h      # NEU
│   │       ├── spacing.cpp/.h         # NEU
│   │       ├── animations.cpp/.h      # NEU
│   │       └── theme.cpp/.h            # NEU
│   └── theme/
│       ├── dark_theme.css           # NEU
│       ├── light_theme.css          # NEU
│       └── custom_theme.css         # NEU
├── assets/
│   ├── icons/
│   │   ├── widgets/                # NEUE ICONS
│   │   ├── control_center/         # NEUE ICONS
│   │   └── bar/                    # NEUE ICONS
│   └── wallpapers/
│       └── l-shell-wallpaper-new.png  # NEU
└── design/
    ├── DESIGN-PLAN-2026.md         # DIESES DOKUMENT
    ├── wireframes/                  # NEU
    ├── mockups/                     # NEU
    └── prototypes/                  # NEU
```

---

## 🚀 UMSETZUNGSPLAN

### Phase 1: Vorbereitung (1 Tag)
- [x] Backup erstellen
- [ ] DESIGN-PLAN-2026.md erstellen (DIESES DOKUMENT)
- [ ] Bestehenden Code analysieren und dokumentieren
- [ ] Design-Tools vorbereiten (Figma/Excalidraw)

### Phase 2: Design (2-3 Tage)
- [ ] Wireframes für alle Komponenten erstellen
- [ ] Mockups für Leisten, Control Center, Widgets
- [ ] Farbpalette und Typography finalisieren
- [ ] Animationen und Übergänge definieren
- [ ] Barrierefreiheits-Check

### Phase 3: Reset & Struktur (1 Tag)
- [ ] Alte UI-Implementierung archivieren
- [ ] Neue Verzeichnisstruktur anlegen
- [ ] Design-System-Dateien erstellen (Farben, Typography, etc.)
- [ ] Theme-System aufbauen

### Phase 4: Implementierung (4-6 Tage)
- [ ] Bar-System neu implementieren
- [ ] Control Center neu implementieren
- [ ] Alle Widgets neu implementieren
- [ ] Animationen und Übergänge implementieren
- [ ] Theme-System integrieren

### Phase 5: Testing & Polish (2-3 Tage)
- [ ] Functionality Testing
- [ ] Visual Regression Testing
- [ ] Performance Testing
- [ ] Barrierefreiheits-Testing
- [ ] Benutzerfeedback einarbeiten

### Phase 6: Dokumentation (1 Tag)
- [ ] Entwicklerdokumentation
- [ ] Benutzerdokumentation
- [ ] API-Dokumentation
- [ ] Theme-Anpassungsdokumentation

---

## 📊 ERFOLGSKRITERIEN

### Visuell
- [ ] Modernes, professionelles Design
- [ ] Konsistentes Design-System
- [ ] Hohe visuelle Qualität
- [ ] Gute Lesbarkeit

### Funktional
- [ ] Alle bestehenden Funktionen erhalten
- [ ] Neue Funktionen implementiert
- [ ] Keine Regressionen
- [ ] Gute Performance

### Technisch
- [ ] Sauberer, wartbarer Code
- [ ] Gute Dokumentation
- [ ] Komplette Testabdeckung
- [ ] Einfache Erweiterbarkeit

---

## 🎉 ZUSAMMENFASSUNG

Dieser Design-Plan beschreibt die **komplette Neugestaltung** der L-Shell UI für Fedora mit Hyprland. Das Ziel ist ein **extrem modernes, professionelles und klassiges Design**, das gleichzeitig **alle notwendigen Funktionen** bietet.

### Wichtigste Änderungen:
1. **Kompletter Reset**: Alle bestehenden UI-Elemente werden entfernt
2. **Neues Design-System**: Konsistente Farben, Typography, Abstände
3. **Modernisierte Komponenten**: Leisten, Control Center, alle Widgets
4. **Verbesserte UX**: Intuitive Bedienung, klare Hierarchien
5. **Optimierte Performance**: GPU-beschleunigt, effizient

### Nächste Schritte:
1. DESIGN-PLAN-2026.md finalisieren ✅ (DIESES DOKUMENT)
2. Wireframes und Mockups erstellen
3. Alten Code archivieren
4. Neue Implementierung beginnen

---

**Status**: Design-Plan erstellt - Bereit für Umsetzung
**Datum**: 15. Oktober 2026
**Version**: 1.0
