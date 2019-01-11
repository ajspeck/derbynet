FROM balenalib/raspberry-pi-python:3.7-stretch-run
COPY . /app
RUN apt-get update \
    && apt-get install -y \
           libraspberrypi0 \
           libraspberrypi-bin \
    && rm -rf /var/lib/apt/lists/*

RUN pip install -r /app/requirements-pi.txt --no-cache-dir
WORKDIR /app
EXPOSE 8000
CMD gunicorn --threads 5 --workers 1 -b 0.0.0.0:8000 app:app
