## myCliHanders.cpp Organization

Contains the following blocks of code:

1. Initialization routines used by setup() and this module, such as
   - `initFilters()`
   - `initSensors()`
2. Update routines of local CLI and MegunoLink used by loop(), such as
   - `updateReadings()`
   - `updateStatus()`
   - `updateGraph()`
   - `updateSelectedDisplays()`
3. OLED Initialization and reporting routines, such as
   - `initOLED()`
   - `updateOLED()`
4. Temperature and Humidity CLI Handlers
   - `cliXXX()`  are called by the CLI dispatcher and display results to the console.
   - `mgnXXX()` augment the previous functionality by displaying results to the MegunoLink GUI.

5. CLI command table
   - Lists of all CLI commands. 
   - Can be involved via the console CLI or via he WEB server

### Source code





```

```