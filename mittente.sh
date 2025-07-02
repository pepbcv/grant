#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Uso: $0 <domid>"
  exit 1
fi

DOMID=$1

LOGFILE="mittente_metrics.log"

echo "Stato prima del test:" > "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"
echo "-------------------------------" >> "$LOGFILE"

echo "Avvio mittente..."
sudo ./m7 "$DOMID"

echo "-------------------------------" >> "$LOGFILE"
echo "Stato dopo il test:" >> "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"

echo "Test completato. Risultati salvati in $LOGFILE"