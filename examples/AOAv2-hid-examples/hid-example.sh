sudo usbhid-dump -e all 2>/dev/null \
| awk 'BEGIN{RS=""}{ for(i=3;i<=NF;i++){printf "%s",$i};printf "\n";fflush()}' \
| xargs -n1 -I {} sh -c "echo {} | xxd -r -p | base64 -w0; echo" \
| ./aoa-proxy --port 1-2 --hid