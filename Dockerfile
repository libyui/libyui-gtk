# Use the libyui/devel image as the base
FROM registry.opensuse.org/devel/libraries/libyui/containers/libyui-devel:latest

RUN  zypper install -y gtk3-devel

COPY . /usr/src/app
