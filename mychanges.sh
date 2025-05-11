#!/bin/bash

ROOT_DIR=$(dirname "$0")

rm $ROOT_DIR/assets/scenes/mychanges.csv

echo "Script,main_menu" >> $ROOT_DIR/assets/scenes/mychanges.csv

$ROOT_DIR/build/bin/Converter.exe mychanges
if [ $? -ne 0 ];
then
    echo "Converter failed, exiting..."
    exit 1
fi


$ROOT_DIR/build/bin/RemoteConsole.exe loadFF mychanges
