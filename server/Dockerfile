FROM python:3.7-slim

# Create app directory
WORKDIR /app

# Install app dependencies
COPY requirements.txt ./

RUN pip3 install --upgrade pip
RUN pip3 install -r requirements.txt

RUN apt-get update && apt-get install -y libglib2.0-0 libgl1-mesa-glx && rm -rf /var/lib/apt/lists/*

# Bundle app source
COPY . .

EXPOSE 5000
RUN ["chmod", "+x", "app.py"]
CMD [ "flask", "run","--host","0.0.0.0","--port","5000"]