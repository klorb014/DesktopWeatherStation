#!/usr/bin/env python3

from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
from flask import Flask
from flask import send_file

import os
import cv2
import requests
from time import gmtime, strftime
app = Flask(__name__)


DEGREE_SYMBOL = chr(176)

image_map = {
    "01d": "Icons/01d.bmp",
    "02d": "Icons/02d.bmp",
    "03d": "Icons/03d.bmp",
    "04d": "Icons/04d.bmp",
    "09d": "Icons/09d.bmp",
    "10d": "Icons/10d.bmp",
    "11d": "Icons/11d.bmp",
    "13d": "Icons/13d.bmp",
    "50d": "Icons/50d.bmp",
    "01n": "Icons/01d.bmp",
    "02n": "Icons/02d.bmp",
    "03n": "Icons/03d.bmp",
    "04n": "Icons/04d.bmp",
    "09n": "Icons/09d.bmp",
    "10n": "Icons/10d.bmp",
    "11n": "Icons/11d.bmp",
    "13n": "Icons/13d.bmp",
    "50n": "Icons/50d.bmp"}


def serve_pil_image(pil_img):
    img_io = BytesIO()
    pil_img.save(img_io, 'BMP')
    img_io.seek(0)
    return send_file(img_io, mimetype='image/bmp')


def create_weather_image(weather: dict) -> Image:

    im2 = Image.open(image_map[weather["icon"]])
    img = Image.new('RGB', (250, 122), "White")  # Create a new black image

    # get a drawing context
    draw = ImageDraw.Draw(img)
    font_path = os.path.join(cv2.__path__[0], 'qt', 'fonts', 'DejaVuSans.ttf')
    font_path_bold = os.path.join(
        cv2.__path__[0], 'qt', 'fonts', 'DejaVuSans-Bold.ttf')
    print(font_path)
    font_small = ImageFont.truetype(font_path_bold, size=15)
    font_big = ImageFont.truetype(font_path, size=30)

    # draw text
    draw.text((0, 0), "{0}\n{1}".format(
        weather["location"], weather["time"]), fill=(0, 0, 0), font=font_small)

    response = requests.get(
        "https://api.openweathermap.org/img/w/{0}.png".format(weather["icon"]))
    icon = Image.open(BytesIO(response.content))
    img.paste(im2, box=(0,38))
    #img.paste(icon, box=(0, 38))

    draw.text((55, 45), "{0}{1}C".format(
        round(weather["temp"]), DEGREE_SYMBOL), fill=(0, 0, 0), font=font_big)

    draw.text((0, 85), "feels like {0}{1}C\n{2}".format(round(
        weather["feels_like"]), DEGREE_SYMBOL, weather["main"]), fill=(0, 0, 0), font=font_small)

    return img


@app.route('/')
def home():
    data = fetch_weather_data("Ottawa", "CA")
    return serve_pil_image(create_weather_image(data))


def fetch_weather_data(city: str, country_code: str) -> dict:
    response = requests.get("http://api.openweathermap.org/data/2.5/weather", params={
                            "q": "{0},{1}".format(city, country_code), "units": "metric", "appid": API_TOKEN})
    data = response.json()
    weather = {}
    weather["location"] = "{0}, {1}".format(
        data["name"], data["sys"]["country"])
    weather["main"] = data["weather"][0]["main"]
    weather["icon"] = data["weather"][0]["icon"]
    weather["temp"] = data["main"]["temp"]
    weather["feels_like"] = data["main"]["feels_like"]
    weather["time"] = strftime(
        "%I:%M %p %y/%m/%d", gmtime(data["dt"] + data["timezone"]))
    print(weather)

    return weather


@app.route('/weather')
def weather():
    response = requests.get("http://api.openweathermap.org/data/2.5/weather", params={
                            "q": "{0},{1}".format("Ottawa", "CA"), "units": "metric", "appid": API_TOKEN})
    weather = response.json()
    data = {}
    data["location"] = "{0}, {1}".format(
        weather["name"], weather["sys"]["country"])
    data["main"] = weather["weather"][0]["main"]
    data["icon"] = weather["weather"][0]["icon"]
    data["temp"] = weather["main"]["temp"]
    data["feels_like"] = weather["main"]["feels_like"]
    data["time"] = strftime("%I:%M:%S %p %y/%m/%d",
                            gmtime(weather["dt"] + weather["timezone"]))

    print(data)
    return response.text


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
