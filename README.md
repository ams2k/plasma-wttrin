# <u>plasma-wttrin</u>

Provider de clima para o applet nativo de previsão do tempo do KDE Plasma 6.7.x.

Utiliza respostas do serviço do site <b>wttr.in<b>

Experimente no terminal:<br>
curl wttr.in/barretos?lang=pt<br>
curl wttr.in/barretos?format=3

## Dependências

No Arch Linux / Manjaro, instale os pacotes de desenvolvimento correspondentes:
```
- `cmake`
- `extra-cmake-modules`
- `qt6-base`
- `kf6-kcoreaddons`
- `kf6-i18n`
- `kf6-unitconversion`
- `kdeplasma-addons` (headers e bibliotecas PlasmaWeather)
```

## Compilar

-- Se a pasta <b>build/</b> existir, remova-a.

rm -rf build
<br>
```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DQT_DEFAULT_MAJOR_VERSION=6 \
    -DKDE_INSTALL_PLUGINDIR=/usr/lib/qt6/plugins \
    -DECM_DIR=/usr/share/ECM/cmake
    
cmake --build build -j"$(nproc)"
```
-- Verificar se há dependências.
```bash
ldd build/lib/plasma/weather_ions/plasma-wttrin.so
```

## Instalar

```bash
sudo cmake --install build
```

-- Confira se o arquivo foi instalado:

```bash
ls -lh /usr/lib/qt6/plugins/plasma/weather_ions/plasma-wttrin.so
```

-- O destino padrão foi configurado para `/usr/lib/qt6/plugins/plasma/weather_ions`,
conforme a instalação informada. <br>&nbsp;&nbsp;&nbsp;Se o diretório for diferente:

```bash
cmake -S . -B build \
  -DWEATHER_ION_INSTALL_DIR=/caminho/qt6/plugins/plasma/weather_ions
```

-- Depois da instalação, reinicie o Plasma Shell ou encerre e inicie a sessão.

## Remover

```bash
sudo rm -f /usr/lib/qt6/plugins/plasma/weather_ions/plasma-wttrin.so
```
<br>
## Observações

- O plugin usa os headers modernos encontrados em `/usr/include/plasma/weather`.
- `findPlaces()` consulta o JSON `j1` do wttr.in e usa `nearest_area` como resultado de localização.
- `fetchForecast()` converte observação atual e previsão diária para os tipos `Forecast`, `Station`,
  `MetaData`, `LastObservation` e `FutureDays`.

## Pasta com fontes

```bash
plasma-wttrin/
├── CMakeLists.txt
├── README.md
└── distrib/
    ├── install.sh
    ├── plasma-wttrin.so
    └── README.md
└── src/
    ├── wttrin.cpp
    ├── wttrin.h
    └── plasma-wttrin.json
```

## Como utilizar o plugin
   
-- Editar o arquivo e ajuste o <b>provider</b> para <b>plasma-wttrin</b>: 

kate ~/.config/plasma-org.kde.plasma.desktop-appletsrc

```
[Containments][2][Applets][44][Configuration][WeatherStation]
placeDisplayName=Barretos
placeInfo=Barretos, Sao Paulo, Brazil
provider=plasma-wttrin
```

-- Na bandeja de aplicativos, se já existir o ícone do widget do clima/tempo,<br>
&nbsp;&nbsp;&nbsp;clique com o botão direito do mouse e Configurar Previsão do Tempo.
