setup=$1; shift
dataset=$1; shift

cfg=${setup}/analysis.toml
dev=${setup}/device.toml
# e.g. -n 10000, to process only the first 10k events
flags="-c ${cfg} -d ${dev} $@"

datasetdir=${setup}/${dataset}
data=data/${setup}/${dataset}.root
output_prefix=output/${setup}/${dataset}/

mkdir -p ${output_prefix}
