#IQRF Daemon Image for the UP board

FROM resin/up-board-debian:latest
MAINTAINER Rostislav Spinar "rostislav.spinar@microrisc.com"

# update repos
RUN echo "deb http://repos.iqrfsdk.org/debian jessie testing" | sudo tee -a /etc/apt/sources.list.d/iqrf-daemon.list \
    && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F973CFCE6B3BE25C

# install daemon
RUN	apt-get update \
	&& apt-get install -y iqrf-daemon \
	&& apt-get clean \
	&& rm -rf /var/lib/apt/lists/*

# run the daemon service 
ENTRYPOINT /usr/bin/iqrf_startup /etc/iqrf-daemon/config.json
