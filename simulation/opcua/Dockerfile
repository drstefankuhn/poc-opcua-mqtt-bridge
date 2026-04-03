FROM python:3.12.13-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt \
    && useradd --create-home --shell /bin/bash --uid 1000 --user-group app

USER app
