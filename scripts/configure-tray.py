# Zone de notification du dock : seulement les icônes d'applis (+ notifications),
# icônes à la taille de la barre, espacement normal. À lancer Plasma arrêté.
import re, sys
p = sys.argv[1]; lines = open(p).read().split("\n")
groups, cur = {}, None
for l in lines:
    m = re.match(r"^\[(.*)\]$", l)
    if m: cur = l; groups.setdefault(cur, []); continue
    if cur: groups[cur].append(l)
arc_ids = {re.fullmatch(r"\[Containments\]\[(\d+)\]", g).group(1)
           for g, b in groups.items() if re.fullmatch(r"\[Containments\]\[(\d+)\]", g)
           and "plugin=org.softyoda.arcpanel" in b}
trays = [g for g, b in groups.items() if "plugin=org.kde.plasma.systemtray" in b
         and re.fullmatch(r"\[Containments\]\[\d+\]\[Applets\]\[\d+\]", g)
         and g.split("][", 2)[1] in arc_ids]
keep = {"org.kde.plasma.cameraindicator"}
known = ["org.kde.kdeconnect","org.kde.plasma.bluetooth","org.kde.plasma.cameraindicator","org.kde.plasma.clipboard",
         "org.kde.plasma.devicenotifier","org.kde.plasma.manage-inputmethod","org.kde.plasma.mediacontroller",
         "org.kde.plasma.notifications","org.kde.plasma.keyboardindicator","org.kde.plasma.weather","org.kde.kscreen",
         "org.kde.plasma.battery","org.kde.plasma.brightness","org.kde.plasma.dwellclicker","org.kde.plasma.keyboardlayout",
         "org.kde.plasma.networkmanagement","org.kde.plasma.volume","org.kde.plasma.printmanager"]
out, skip = [], False
for l in lines:
    if re.match(r"^\[(.*)\]$", l):
        skip = False
        for t in trays:
            if l.startswith(t + "[Applets]["):
                plug = next((b.split("=", 1)[1] for b in groups.get(l, []) if b.startswith("plugin=")), "")
                if plug and plug not in keep: skip = True
            if l == t + "[General]": skip = True
    if not skip: out.append(l)
while out and not out[-1].strip():
    out.pop()
for t in trays:
    out += ["", t + "[General]", "extraItems=org.kde.plasma.cameraindicator", "knownItems=" + ",".join(known),
            "scaleIconsToFit=true", "iconSpacing=2"]
open(p, "w").write("\n".join(out) + "\n")
print("zones de notif :", trays)
