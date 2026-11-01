# The original deterministic EvoLab Ω C++17 engine is compiled, not simulated in JS.
FROM node:22-bookworm AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ make && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY native/omega_core.cpp native/build.sh /app/native/
RUN bash /app/native/build.sh

FROM node:22-bookworm-slim
ENV NODE_ENV=production HOST=0.0.0.0 PORT=10000 DATA_ROOT=/app/data MAX_ACTIVE=1 MAX_WALL_SECONDS=480
WORKDIR /app
COPY --from=build /app/native/omega_core /app/native/omega_core
COPY native/controller.js /app/native/controller.js
COPY *.js *.css *.html *.json *.png /app/
COPY evidence /app/evidence
RUN mkdir -p /app/data && chown -R node:node /app/data && chmod 755 /app/native/omega_core
USER node
EXPOSE 10000
CMD ["node", "public-server.js"]
