FROM debian:stretch AS builder
COPY . /derbynet
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y \
        openjdk-8-jdk-headless \
        ant \
        git
RUN cd /derbynet && ant dist-nodoc.zip
RUN mv /*.zip /derbynet.zip

FROM openjdk:8-jre-stretch

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && \ 
    apt-get install -y \
	nano \
	zip unzip \
	wget \
	curl \
	imagemagick \
	sqlite sqlite3 \
	php7.0 \
	php7.0-fpm \
	php7.0-cli \
	php7.0-json \
	php7.0-curl \
	php7.0-gd \
	php-imagick \
	php7.0-sqlite3 \
	php7.0-pgsql \
	php7.0-mysql \
	php7.0-imap \
	php7.0-xsl \
	php7.0-ldap \
	php-memcache \
	php7.0-mcrypt \
	#php-xdebug \
	php7.0-intl \
	#php7.0-xmlrpc \
	php7.0-xml \
	php-pear \
	php-geoip \
	php7.0-opcache \
	php-gettext \
	php7.0-zip \
	--no-install-recommends && \
	rm -rf /var/lib/apt/lists/*

RUN mkdir -p /tmp/derbynet/
COPY --from=builder /derbynet.zip /tmp/derbynet/derbynet.zip
RUN unzip /tmp/derbynet/derbynet.zip -d /tmp/derbynet
RUN	mkdir -p /var/www/html && chown -R www-data:www-data /var/www
	
RUN sed -i -e 's/error_log = \/var\/log\/php7.0-fpm.log/error_log = \/proc\/self\/fd\/2/g' /etc/php/7.0/fpm/php-fpm.conf && \
	sed -i -e 's/;daemonize = yes/daemonize = no/g' /etc/php/7.0/fpm/php-fpm.conf && \
	sed -i -e 's/;access.log = log\/$pool.access.log/access.log = \/proc\/self\/fd\/2/g' /etc/php/7.0/fpm/pool.d/www.conf && \
	sed -i -e 's/;catch_workers_output = yes/catch_workers_output = yes/g' /etc/php/7.0/fpm/pool.d/www.conf && \
	sed -i -e 's/;clear_env = no/clear_env = no/g' /etc/php/7.0/fpm/pool.d/www.conf && \
	sed -i -e 's/listen = \/run\/php\/php7.0-fpm.sock/listen = [::]:9000/g' /etc/php/7.0/fpm/pool.d/www.conf && \
	mkdir -p /run/php

COPY ./docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

VOLUME /etc/php/7.0/fpm/pool.d/ /var/lib/php/sessions
EXPOSE 9000

ENTRYPOINT ["/entrypoint.sh"]
CMD ["php-fpm7.0"]

ENV NGINX_VERSION 1.10.*

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update \
	&& apt-get install -y \
              ca-certificates \
              nginx=${NGINX_VERSION} \
              php-curl \
              php-gd \
              php-sqlite3 \
              unzip


RUN mv /tmp/derbynet/racemgr /var/www/html/derbynet
RUN mkdir -p /usr/share/derbynet/
RUN mv /tmp/derbynet/extras /usr/share/derbynet/
RUN mv /tmp/derbynet/derby-timer.jar /usr/bin/
COPY ./docker/location.snippet /etc/nginx/derbynet/location.snippet
COPY ./docker/index.html /var/www/html/index.html
COPY ./docker/derbynet.conf /etc/derbynet.conf
COPY ./docker/timer-start.sh /timer-start.sh
RUN chmod +x /timer-start.sh
RUN ln -sf /dev/stdout /var/log/nginx/access.log \
	&& ln -sf /dev/stderr /var/log/nginx/error.log
	
RUN sed -i "s/worker_processes auto;/worker_processes 1;/g" /etc/nginx/nginx.conf
RUN mkdir -m 777 /var/lib/derbynet
RUN chown www-data.www-data /var/lib/derbynet
RUN ls -l /var/www/html/
RUN ls -l /var/www/html/derbynet/
RUN mkdir -m 777 /var/www/html/derbynet/local
COPY ./docker/default-file-path.inc /var/www/html/derbynet/local/default-file-path.inc
RUN sed -i -e '/^[ \t]*location \/ {/ i\ include derbynet/location.snippet;' /etc/nginx/sites-available/default
VOLUME /var/lib/derbynet/
