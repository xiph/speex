#!/bin/sh

set -e
set -u

MALE_SAMPLE=male.wav
FEMALE_SAMPLE=female.wav
WB_MALE_SAMPLE=wb_male.wav
UWB_MALE_SAMPLE=uwb_male.wav
OUTPUT_DIR=samples
mkdir -p ${OUTPUT_DIR}

samples="${MALE_SAMPLE} ${FEMALE_SAMPLE} ${WB_MALE_SAMPLE}"
for sample in $samples; do
    wget --no-clobber https://www.speex.org/samples/audio/${sample}
done

# FIXME: move to speex.org
wget --no-clobber https://people.xiph.org/~tterribe/speex/${UWB_MALE_SAMPLE}

bitrates="4 8 11 15"
for b in $bitrates; do
    kbitrate=$(expr $b \* 1000)
    ./speexenc --narrowband --bitrate ${kbitrate}  ${MALE_SAMPLE} ${OUTPUT_DIR}/male_speex_${b}.spx
    ./speexenc --narrowband --bitrate ${kbitrate} --vbr ${MALE_SAMPLE} ${OUTPUT_DIR}/male_speex_${b}_vbr.spx
    ./speexenc --narrowband --bitrate ${kbitrate} ${FEMALE_SAMPLE} ${OUTPUT_DIR}/female_speex_${b}.spx
    ./speexenc --narrowband --bitrate ${kbitrate} --vbr ${FEMALE_SAMPLE} ${OUTPUT_DIR}/female_speex_${b}_vbr.spx
done

bitrates="10 12 17 18 21 28"
for b in $bitrates; do
    kbitrate=$(expr $b \* 1000)
    ./speexenc --wideband --bitrate ${kbitrate}  ${WB_MALE_SAMPLE} ${OUTPUT_DIR}/wb_male_speex_${b}.spx
    ./speexenc --wideband  --bitrate ${kbitrate} --vbr ${WB_MALE_SAMPLE} ${OUTPUT_DIR}/wb_male_speex_${b}_vbr.spx
done

bitrates="10 12 17 18 21 28"
for b in $bitrates; do
    kbitrate=$(expr $b \* 1000)
    ./speexenc --ultra-wideband --bitrate ${kbitrate} ${UWB_MALE_SAMPLE} ${OUTPUT_DIR}/uwb_male_speex_${b}.spx
    ./speexenc --ultra-wideband --bitrate ${kbitrate} --vbr ${UWB_MALE_SAMPLE} ${OUTPUT_DIR}/uwb_male_speex_${b}_vbr.spx
done
