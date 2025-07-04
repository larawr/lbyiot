from flask import Flask, jsonify
import requests
import datetime

app = Flask(__name__)
latest_count = 0

SUPABASE_URL = "https://YOUR_PROJECT.supabase.co/rest/v1/rep_data"
SUPABASE_API_KEY = "YOUR_ANON_KEY"

@app.route('/update/<int:count>')
def update(count):
    global latest_count
    latest_count = count

    # Send to Supabase
    headers = {
        "apikey": SUPABASE_API_KEY,
        "Authorization": f"Bearer {SUPABASE_API_KEY}",
        "Content-Type": "application/json",
        "Prefer": "return=minimal"
    }

    data = {
        "count": count
    }

    try:
        r = requests.post(SUPABASE_URL, headers=headers, json=data)
        if r.status_code in [200, 201, 204]:
            print(f"✔ Sent to Supabase: {count}")
        else:
            print("❌ Supabase error:", r.text)
    except Exception as e:
        print("⚠️ Exception:", e)

    return jsonify({"message": "updated", "count": latest_count})

@app.route('/latest-count')
def latest():
    return jsonify({"count": latest_count})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
