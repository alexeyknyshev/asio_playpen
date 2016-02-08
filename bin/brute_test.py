import requests

while True:
    r = requests.get('http://localhost:8083/?url=http%3A%2F%2Flocalhost%3A8082%2Ftest')
#print(r.status_code)
