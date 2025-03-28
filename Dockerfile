FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \ 
    libomp-dev \
    openmpi-bin \
    libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY emergency_simulation.c .

RUN mpicc -fopenmp -o emergency_simulation emergency_simulation.c -lm

ENTRYPOINT ["mpirun", "--allow-run-as-root", "-np", "4", "./emergency_simulation"]
