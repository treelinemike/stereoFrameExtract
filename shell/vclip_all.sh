#!/bin/bash
set -e  # allow errors
for yaml_filename in *.yaml; do
  bash "vclip -y $yaml_filename"
done
