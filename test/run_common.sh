path=$1; shift
# assumes data path is <setup>/<dataset>
setup=$(echo ${path} | cut -f1 -d'/' -)

cfg=${setup}/analysis.toml
dev=${setup}/device.toml
# e.g. -n 10000, to process only the first 10k events
flags="-c ${cfg} -d ${dev} $@"

datasetdir=${path}
data=data/${path}.root
output_prefix=output/${path}/

mkdir -p ${output_prefix}
