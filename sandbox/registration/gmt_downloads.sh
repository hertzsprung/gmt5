#!/bin/bash
# Prepare data tables for looking at download statistics
# Based on the latest data dump from gmtserver
scp root@gmtserver.soest.hawaii.edu:/opt/bitnami/rubystack-3.2.3-0/apps/files/db/production.sqlite3.dump .
RAW=production.sqlite3.dump
cat << EOF > fix.txt
s/INSERT INTO "request_ips" VALUES//g
s/'//g
EOF
# Make a file with lon, lat, time, country, file, IP, address
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gmt-5.1.1- | sed -f fix.txt | awk -F, '{printf "%s\t%s\t%sT%s\t%s\t%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $4, $8, $7}' > all.lis
awk '{printf "nslookup -timeout=4 %s\n", $NF}' all.lis | sh -s | grep "in-addr.arpa" > addresses.lis
wc all.lis addresses.lis
exit
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gmt-5.1.1-src.tar.bz2 | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > src.lis
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gmt-5.1.1-darwin-x86_64.dmg | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > mac.lis
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gmt-5.1.1-win32.exe | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > win32.lis
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gmt-5.1.1-win64.exe | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > win64.lis
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep gshhg-gmt | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > gshhg.lis
grep 'INSERT INTO "request_ips"' $RAW | tr 'A-Z' 'a-z' | grep dcw-gmt | sed -f fix.txt | awk -F, '{if ($NF != last && $4 == "us") {printf "%s\t%s\t%sT%s\t%s\n", $2, $3, substr($9,1,10), substr($9,12), $7; last = $NF}}' > dcw.lis
