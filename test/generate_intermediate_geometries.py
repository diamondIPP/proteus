#!/usr/bin/env python3
#
# Generate intermediate geometry files from the initial and optimal geometry.

import pathlib
import sys
from pathlib import Path

import toml

def main():
    # each argument must the the path to a dataset directory
    for arg in sys.argv[1:]:
        generate_geometries(pathlib.Path(arg))

def generate_geometries(dataset_dir):
    """
    Generate a set of new geometries from existing ones in the dataset directory.
    """
    geo_optimal = toml.load(dataset_dir / 'geometry.toml')
    geo_initial = toml.load(dataset_dir / 'geometry-initial.toml')
    # output name and sensor ids which should use the initial geometry
    what_to_generate = [
        # optimal telescope geometry
        ('telescope', [1, 2, 3, 4, 5, 6]),
        # optimal geometry for first and last telescope plane
        ('telescope_first_last', [1, 6]),
    ]
    for name, optimal_sensor_ids in what_to_generate:
        merged = merge_geometries(geo_optimal, geo_initial, optimal_sensor_ids)
        path = dataset_dir / 'geometry-{}.toml'.format(name)
        with path.open('wt', encoding='utf-8') as f:
            toml.dump(merged, f)
            print('wrote {}'.format(path))

def merge_geometries(geo_optimal, geo_initial, optimal_sensor_ids,
                     use_optimal_beam=True):
    """
    Merge two geometry configurations.
    """
    # pick one beam configuration
    beam = geo_optimal['beam'] if use_optimal_beam else geo_initial['beam']
    # merge sensor configs by picking some from either side
    optimal = geo_optimal.get('sensors', [])
    initial = geo_initial.get('sensors', [])
    sensors = merge_sensors(optimal, initial, optimal_sensor_ids)
    return {'beam': beam, 'sensors': sensors}

def merge_sensors(sensors0, sensors1, ids_from_first):
    """
    Merge two sensor lists by taking some from the first and reston from second.
    """
    sensors = []
    for sensor in sensors0:
        if sensor['id'] in ids_from_first:
            sensors.append(sensor)
    for sensor in sensors1:
        if sensor['id'] not in ids_from_first:
            sensors.append(sensor)
    sensors.sort(key=lambda _: _['id'])
    return sensors

if __name__ == '__main__':
    main()
