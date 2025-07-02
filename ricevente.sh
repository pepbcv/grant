#!/bin/bash

DOMIDA=$1
GREF=$2

LOGFILE="ricevente_metrics.log"

echo "Stato prima del test:" > "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"
echo "-------------------------------" >> "$LOGFILE"

echo "Avvio ricevente..."
sudo ./r7 "$DOMIDA" "$GREF"

echo "-------------------------------" >> "$LOGFILE"
echo "Stato dopo il test:" >> "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"

echo "Test completato. Risultati salvati in $LOGFILE"