# syntax=docker/dockerfile:1
FROM alpine
ARG COCAINE_DIESEL_VERSION
RUN mkdir /cocainediesel
WORKDIR /cocainediesel
COPY release/headlessupdater /cocainediesel/
RUN ./headlessupdater ${COCAINE_DIESEL_VERSION}
RUN rm headlessupdater headlessupdater.old version.txt manifest.txt

FROM scratch
COPY --from=0 /cocainediesel/ /
EXPOSE 4050/tcp
EXPOSE 4050/udp
ENTRYPOINT ["/server"]
