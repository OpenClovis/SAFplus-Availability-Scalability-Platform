#!/bin/bash

# Generate On-line API Ref Guide and upload it to intranet server
./post_searchable_apiref.sh

# Upload normal user documentation to intranet server as <URL>/userdocs
./post_userdocs.sh

# Generate HTML on-line source code browser and upload it to intranet server
./post_html_source_code_view.sh

# Setup 'latest' soft-link on the build server to point to latest build
ssh -l root 192.168.0.94 \
    "cd /home2/ftp/pub/eng-builds; ln -fs openclovis-sdk-3.0beta-$BUILD_NUMBER.tar.gz openclovis-sdk-3.0beta-latest.tar.gz; ln -fs openclovis-sdk-3.0beta-$BUILD_NUMBER.md5 openclovis-sdk-3.0beta-latest.md5"

