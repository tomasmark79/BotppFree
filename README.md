[![Linux](https://github.com/tomasmark79/BotppFree/actions/workflows/linux.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/linux.yml)
[![MacOS](https://github.com/tomasmark79/BotppFree/actions/workflows/macos.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/macos.yml)
<!-- [![Windows](https://github.com/tomasmark79/BotppFree/actions/workflows/windows.yml/badge.svg)](https://github.com/tomasmark79/BotppFree/actions/workflows/windows.yml)   -->

# BotppFree

Univerzální Discord bot.
 
 > Cílem bota je v tuto dobu především servírování novinek z definovaných zdrojů RSS do specifického kanálu

Bot v tuto dobu běží v testovacím módu na největším Discord Serveru Linux CZ/SK - [Pozvánka](https://discord.gg/MBuvrRWQR6)

 - project je open source, a je možné jej použít kýmkoli a kdekoli
 - stav projektu je nyní ve velmi ranné fázi testování

### Některé vlastnosti

- obsahuje vlastní paměťovou datovou strukturu v podobě **fronty**
- přidávání do fronty se provádí v samostatném vlákně s intervalem 2 hodin
- pro zamezení redundance záznamů se při vyzvednutí záznamu z fronty záznam odstraní
- výtisk náhodných zpráv do definovaného kanálu se děje v intervalu 10 minut
- pokud je fronta prázdná, nevypíše se nic a čeká se na nové krmivo
 
### ToDo

  - etxerní příkazy pro fetch feed jsou do odvolání vypnuty
  - definovat rss deklarace v externím json
  - ukládat seenHashes do externího json (v produkci výtisk vždy bez duplikátů)

 ---

BotppFree is proudly built with **[D🌀tName C++ Template](https://github.com/tomasmark79/DotNameCppFree)**.

## License

MIT License  
Copyright (c) 2024-2025 Tomáš Mark

## Disclaimer

This template is provided "as is," without any guarantees regarding its functionality or suitability for any purpose.