#!/usr/bin/env python3
"""Read one complete OoT-RenderFlight-*.log; summarize continuous evidence and gaps.

No third-party dependencies. Raw records remain the authority; CPU submission
intervals are not physical display timestamps, and nested costs are not additive.
"""
import argparse
import collections
import json
import math
from pathlib import Path


def percentile(values, fraction):
    if not values:
        return None
    return sorted(values)[max(0, math.ceil(fraction * len(values)) - 1)]


def capture_tables(renders, ticks, errors):
    """Merge every retained identity; source residency is last observed, never summed."""
    resources, textures, passes, actors, stage_groups = {}, {}, {}, {}, {}
    unknown = collections.Counter()
    context_fields = ('scene', 'room', 'age', 'alt_assets', 'paused')

    def target(table, context, entry, identity=('path',)):
        key = tuple(context.get(k) for k in context_fields) + tuple(entry.get(k) for k in identity)
        if key not in table:
            table[key] = {**{k: context.get(k) for k in context_fields},
                          **{k: entry.get(k) for k in identity}}
        return table[key]

    def costs(destination, entry):
        destination['command_calls'] = destination.get('command_calls', 0) + entry.get('calls', 0)
        destination['command_ms'] = destination.get('command_ms', 0) + entry.get('ms', 0)
        for field in ('self', 'texture_import', 'driver_draw', 'shader_create', 'upload'):
            value = entry.get(field)
            if value is None:
                destination.setdefault(field + '_ms', None)
            else:
                destination[field + '_ms'] = (destination.get(field + '_ms') or 0) + value.get('ms', 0)
                destination[field + '_calls'] = destination.get(field + '_calls', 0) + value.get('calls', 0)
        destination['self_unknown_commands'] = destination.get('self_unknown_commands', 0) + entry.get('self_unknown_commands', 0)

    for render in renders:
        for name in ('resource', 'texture', 'pass'):
            errors['renderer_' + name + '_overflow'] += render.get(name + '_overflow', 0)
        errors['renderer_self_accounting_clamps'] += render.get('self_accounting_clamps', 0)
        errors['renderer_invalid_opcodes'] += render.get('invalid_opcodes', 0)
        errors['renderer_emission_truncated'] += bool(render.get('emission_truncated', False))
        if render.get('schema', 1) < 2:
            errors['legacy_renderer_tables'] += 1
        else:
            for field in ('resources', 'textures', 'passes'):
                if not isinstance(render.get(field), list):
                    errors['missing_renderer_tables'] += 1
            for prefix in ('resource', 'texture', 'pass'):
                if prefix + '_keys' not in render or prefix + '_limit' not in render:
                    errors['missing_renderer_limits'] += 1
            for field, prefix in (('resources', 'resource'), ('textures', 'texture'), ('passes', 'pass')):
                # One reserved unknown bucket can legitimately have no observations.
                minimum = max(0, render.get(prefix + '_keys', 0) - 1)
                errors['renderer_table_rows_missing'] += max(0, minimum - len(render.get(field, [])))
        for name in ('unknown_resource_selections', 'unknown_texture_selections', 'unmarked_resource_commands'):
            unknown[name] += render.get(name, 0)
        for entry in render.get('resources', render.get('top_resources', [])):
            costs(target(resources, render, entry), entry)
        for entry in render.get('passes', []):
            costs(target(passes, render, entry, ('path', 'framebuffer')), entry)
        for entry in render.get('textures', render.get('top_texture_uploads', [])):
            row = target(textures, render, entry)
            row['upload_calls'] = row.get('upload_calls', 0) + entry.get('calls', 0)
            row['upload_ms'] = row.get('upload_ms', 0) + entry.get('ms', 0)
            row['upload_bytes'] = row.get('upload_bytes', 0) + entry.get('bytes', 0)
            row['source_resident_bytes_last'] = entry.get('source_resident_bytes')
            row['source_data_bytes_last'] = entry.get('source_data_bytes')
            row['source_backing_bytes_last'] = entry.get('source_backing_bytes')
            row['decoded_resident_bytes'] = entry.get('decoded_resident_bytes')
            for field in ('source_width', 'source_height', 'source_format', 'source_flags', 'upload_width', 'upload_height'):
                row[field + '_last'] = entry.get(field)
            for field in ('cache_hits', 'cache_misses', 'metadata_changes', 'upload_dimension_changes', 'upload_dimensions_unknown'):
                row[field] = row.get(field, 0) + entry.get(field, 0)
    for tick in ticks:
        trace = tick.get('trace') or {}
        for field in ('actor_keys_omitted', 'aggregate_keys_omitted', 'overflow_resources', 'overflow_resource_stages',
                      'truncated_names', 'truncated_categories', 'truncated_details'):
            errors[field] += trace.get(field, 0)
        unknown['trace_unknown_names'] += trace.get('unknown_names', 0)
        for entry in trace.get('actor_aggregates', []):
            row = target(actors, tick, entry, ('name', 'manager', 'cache_owner'))
            for field in ('update_count', 'draw_count', 'update_ns', 'draw_ns'):
                row[field] = row.get(field, 0) + entry.get(field, 0)
            for field in ('update_max_ns', 'draw_max_ns'):
                row[field] = max(row.get(field, 0), entry.get(field, 0))
        for entry in trace.get('resource_aggregates', []):
            row = target(stage_groups, tick, entry, ('name', 'manager', 'cache_owner'))
            row['origin_tick_last'] = entry.get('origin_tick')
            stages = row.setdefault('stages', {})
            for stage, value in entry.get('stages', {}).items():
                combined = stages.setdefault(stage, {'count':0, 'total_ns':0, 'max_ns':0})
                combined['count'] += value.get('count', 0)
                combined['total_ns'] += value.get('total_ns', 0)
                combined['max_ns'] = max(combined['max_ns'], value.get('max_ns', 0))
    return {'renderer_resources':list(resources.values()), 'textures':list(textures.values()),
            'framebuffer_passes':list(passes.values()), 'actors':list(actors.values()),
            'resource_stages':list(stage_groups.values()), 'unknown_attribution':dict(unknown),
            'semantics':'all retained identities; CPU elapsed includes waits; inclusive operation/stage times overlap; '
                        'source payload/backing sizes are last observed per path, not a unique engine/GPU footprint'}


def analyze(text):
    attempts, ticks, renders, configurations, tails = [], [], [], [], []
    errors = collections.Counter()
    for line in text.splitlines():
        if '[RenderFlightState] ' in line: errors["capture_limit_events"] += 1
        for marker, output in [("[FrameFlightRecorder] ", None), ("[RenderCostProbe] ", renders),
                               ("[RenderFlightConfiguration] ", configurations), ("[RenderFlightTail] ", tails)]:
            if marker not in line:
                continue
            try:
                data = json.loads(line.split(marker, 1)[1])
            except (ValueError, IndexError):
                errors['malformed_records'] += 1
                continue
            if output is not None:
                output.append(data)
            else:
                attempts.extend(data.get('attempts', [])); ticks.extend(data.get('ticks', []))
                for key in ['attempt_overflow', 'tick_overflow']:
                    errors[key] += data.get(key, 0)
                errors['diagnostic_errors'] = max(errors['diagnostic_errors'], data.get('diagnostic_errors', 0))
    groups = collections.defaultdict(list)
    for row in attempts:
        groups[(row['scene'], row['room'], row['age'], row['alt_assets'], row['paused'])].append(row)
    summaries = []
    gpu_by_id = {}
    for row in attempts:
        for sample in (row.get('gpu') or {}).get('samples', []):
            if sample.get('gpu_ms') is not None: gpu_by_id[sample['frame_id']] = sample['gpu_ms']
    for key, rows in groups.items():
        rows.sort(key=lambda r: r.get('start_ns', 0))
        drawn = [r for r in rows if r.get('presented')]
        # Reset interval comparison on each context transition or skipped batch gap.
        intervals = []
        for a, b in zip(drawn, drawn[1:]):
            if b.get('context_id') == a.get('context_id'):
                intervals.append((b['end_ns']-a['end_ns'])/1e6)
        repeated = 0
        for a, b in zip(drawn, drawn[1:]):
            if a['tick_id']==b['tick_id'] and a.get('matrix_digest') is not None and a.get('matrix_digest')==b.get('matrix_digest') and a.get('matrix_count',0)>0:
                repeated += 1
        gpu = [gpu_by_id[r['id']] for r in rows if r['id'] in gpu_by_id]
        sums = lambda field: sum(r.get(field, 0) for r in drawn) / len(drawn) if drawn else None
        summaries.append({'scene_room_age_alt_paused': key, 'attempts': len(rows), 'presented':len(drawn),
            'skipped':len(rows)-len(drawn), 'skip_reasons':dict(collections.Counter((r.get('pacing') or {}).get('reason','unavailable') for r in rows if not r.get('presented'))),
            'cpu_submission_interval_ms':{'p50':percentile(intervals,.5),'p95':percentile(intervals,.95),'p99':percentile(intervals,.99),'max':max(intervals,default=None)},
            'commands_ms_mean':sums('commands_ms'), 'present_ms_mean':sums('present_ms'),
            'gpu_ms_p95':percentile(gpu,.95), 'gpu_samples':len(gpu),
            'repeated_matrix_pose_pairs_same_tick':repeated,
            'detail_modes':sorted(set(r.get('detail_mode',-1) for r in rows))})
    resource_events = []
    for tick in [*ticks, *tails]:
        if tick.get('trace') is None: errors['missing_tick_trace'] += 1
        trace = tick.get('trace') or {}
        for key in ['overflow_events','overflow_aggregates','overflow_counters','overflow_phases','overflow_actors','collector_errors']:
            errors[key] += trace.get(key,0)
        resource_events.extend(e for e in trace.get('events',[]) if e.get('category','').startswith('resource.'))
    errors['incomplete_tail_events'] += sum(bool(tail.get('incomplete_tail_possible')) for tail in tails)
    slow = sorted(ticks,key=lambda r:r.get('wall_ms',0),reverse=True)[:20]
    tables = capture_tables(renders, [*ticks, *tails], errors)
    return {'coverage':{'configurations':len(configurations),'attempts':len(attempts),'ticks':len(ticks),
                       'detailed_draws':len(renders),'resource_events':len(resource_events),'tail_records':len(tails),'errors':dict(errors)},
            'scenes':summaries,'slow_ticks':slow,
            'resource_capture':tables,
            'slow_resource_events':sorted(resource_events,key=lambda e:e.get('duration_ns',0),reverse=True)[:30],
            'semantics':'CPU submission intervals, not scanout; GPU results retain source frame IDs; nested phases overlap; unmatched camera/content prevents causal before/after claims'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    parser.add_argument('--require-complete',action='store_true')
    args=parser.parse_args()
    result=analyze(args.capture.read_text(errors='replace'))
    print(json.dumps(result,indent=2))
    c=result['coverage']
    return int(args.require_complete and (not c['attempts'] or not c['ticks'] or not c['configurations'] or any(c['errors'].values())))

if __name__=='__main__':
    raise SystemExit(main())
