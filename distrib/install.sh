#!/bin/bash

set -e

PLUGIN="plasma-wttrin.so"
DEST="/usr/lib/qt6/plugins/plasma/weather_ions"

if [ ! -f "$PLUGIN" ]; then
    echo "Erro: arquivo $PLUGIN não encontrado."
    exit 1
fi

echo "Instalando plugin wttr.in..."

sudo install -D -m 755 "$PLUGIN" "$DEST/$PLUGIN"

echo "Verificando dependências..."

if ldd "$DEST/$PLUGIN" | grep -q "not found"; then
    echo "ATENÇÃO: existem dependências ausentes!"
    ldd "$DEST/$PLUGIN" | grep "not found"
    exit 1
fi

echo "Plugin instalado com sucesso!"
echo "Reinicie a sessão do KDE Plasma."
