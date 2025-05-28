[![Linux](https://github.com/tomasmark79/BotppFree/actions/workflows/linux.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/linux.yml)
[![MacOS](https://github.com/tomasmark79/BotppFree/actions/workflows/macos.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/macos.yml)
<!-- [![Windows](https://github.com/tomasmark79/BotppFree/actions/workflows/windows.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/windows.yml)   -->

# Bot++

Univerzální Discord Bot s podporou RSS čtečky.
 
 > Cílem bota servírování novinek z definovaných zdrojů RSS do specifického kanálu.

Bot v tuto dobu běží v testovacím módu na největším Discord Serveru Linux CZ/SK - [Pozvánka](https://discord.gg/MBuvrRWQR6)

 - project je open source, a je možné jej použít kýmkoli a kdekoli
 - stav projektu je nyní ve velmi ranné fázi testování

### Některé vlastnosti

- obsahuje vlastní paměťovou datovou strukturu v podobě **fronty**
- přidávání do fronty se provádí v samostatném vlákně
- zamezení redundance vystavených článků (i po restartu programu) 
- výtisk náhodných zpráv do definovaného kanálu se děje v samostatném vlákně
- pokud je fronta prázdná, nevypíše se nic a čeká se na nové položky, které RSS zdroj poskytne
- za běhu programu je možné editovat konfiguraci se zdroji RSS
 
### ToDo

  - etxerní příkazy pro fetch feed jsou do odvolání vypnuty

 ---

BotppFree is proudly built with **[D🌀tName C++ Template](https://github.com/tomasmark79/DotNameCppFree)**.

## License

MIT License  
Copyright (c) 2024-2025 Tomáš Mark

## Disclaimer

This template is provided "as is," without any guarantees regarding its functionality or suitability for any purpose.