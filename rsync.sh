#!/usr/bin/env bash
rsync -avh --exclude=.git/ ./ $s210h/RelationClassification-RL/
