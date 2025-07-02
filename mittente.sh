#!/bin/bash

DOMID=$1
LOGFILE="mittente_metrics.log"

echo "=== STATO PRIMA DEL TEST ===" > "$LOGFILE"
date >> "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"
echo "-------------------------------" >> "$LOGFILE"

START=$(date +%s.%N)
sudo ./r7 "$DOMID" >> "$LOGFILE"
END=$(date +%s.%N)

echo "=== STATO DOPO IL TEST ===" >> "$LOGFILE"
date >> "$LOGFILE"
free -h >> "$LOGFILE"
top -b -n1 | grep "Cpu(s)" >> "$LOGFILE"
echo "Durata totale (sec): $(echo "$END - $START" | bc)" >> "$LOGFILE"