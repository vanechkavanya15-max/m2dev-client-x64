import re

path = r'E:\m2dev-client-src-mainOryginalx64\src\UserInterface\TestHarness\TestHarnessServer.cpp'
with open(path, 'r', encoding='latin-1') as f:
    text = f.read()

matches = re.findall(r'm_routes\["([^"]+)"\]', text)
print("Registered routes in TestHarnessServer.cpp:")
for m in sorted(set(matches)):
    print(f"  - {m}")
