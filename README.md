# ios
Operační systémy - obsahuje projekty (Bash/C)

# wana
Náplní prvního projektu byl filtr v Bashi. 

Správné použití: wana [FILTR] [PŘÍKAZ] [LOG [LOG2 [...]]]
Hodnoty filtru
    -a DATETIME - jsou uvažovány pouze hodnoty PO tomtu datu (vyjma). DATETIME je formátu YYYY-MM-DD HH:MM:SS
    -b DATETIME - jsou uvažovány pouze hodnoty PŘED tímto datem (vyjma). DATETIME je formátu YYYY-MM-DD HH:MM:SS
    -ip IPADR - jsou uvažovány záznamy požadavků ze zdrojové adresy IPADDR. IPADDR je formátu IPv4 nebo IPv6
    -uri URI - jsou uvažovány záznamy týkající se dotazů na konkrétní stránku.
    
# druhý projekt
Implementace river-crossing problému v C za použití procesů a semaforů. 
