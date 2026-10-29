#!/usr/bin/env python3
import json
import unittest
from analyze_render_flight import analyze

class FlightAnalysisTest(unittest.TestCase):
    def test_skips_stalls_gpu_correlation_and_context_boundary(self):
        base={'scene':81,'room':0,'age':1,'alt_assets':True,'paused':False,'context_id':1,
              'tick_id':1,'start_ns':0,'end_ns':1000000,'presented':True,'detail_mode':2}
        first={**base,'id':1}
        skip={**base,'id':2,'presented':False,'pacing':{'reason':'scheduler_late'}}
        second={**base,'id':3,'start_ns':400000000,'end_ns':450000000,'tick_id':8,
                'gpu':{'samples':[{'frame_id':1,'gpu_ms':4.5}]}}
        transition={**base,'id':4,'context_id':2,'start_ns':9000000000,'end_ns':9001000000}
        capture={'attempts':[first,skip,second,transition],'ticks':[{'id':1,'wall_ms':458,'trace':{'events':[]}}]}
        result=analyze('[RenderFlightConfiguration] {}\n[FrameFlightRecorder] '+json.dumps(capture))
        scene=result['scenes'][0]
        self.assertEqual(scene['skipped'],1)
        self.assertEqual(scene['cpu_submission_interval_ms']['max'],449.0)
        self.assertEqual(scene['gpu_samples'],1)
        self.assertEqual(scene['gpu_ms_p95'],4.5)
        self.assertEqual(result['slow_ticks'][0]['wall_ms'],458)
    def test_missing_and_truncated_capture(self):
        r=analyze('[RenderFlightState] {"event":"capture_limit"}\n[FrameFlightRecorder] broken')
        self.assertEqual(r['coverage']['attempts'],0)
        self.assertEqual(r['coverage']['errors']['capture_limit_events'],1)
        self.assertEqual(r['coverage']['errors']['malformed_records'],1)

    def test_full_resource_texture_and_actor_tables(self):
        resources = [{'path':f'objects/{i}', 'calls':1, 'ms':2, 'self':{'calls':1,'ms':1},
                      'driver_draw':{'calls':1,'ms':.5}, 'texture_import':{'calls':1,'ms':.5},
                      'upload':{'calls':1,'ms':.1}} for i in range(80)]
        textures = [{'path':f'textures/{i}', 'calls':1, 'ms':.1, 'bytes':4096,
                     'source_data_bytes':2048, 'source_resident_bytes':None, 'decoded_resident_bytes':None,
                     'upload_width':32,'upload_height':32} for i in range(80)]
        actors = [{'name':f'id={i}', 'update_count':1,'draw_count':1,'update_ns':10,'draw_ns':20}
                  for i in range(90)]
        render = {'schema':2, 'resources':resources, 'textures':textures, 'passes':[],
                  'resource_keys':81,'texture_keys':81,'pass_keys':1,
                  'resource_limit':512,'texture_limit':512,'pass_limit':128,'emission_truncated':False}
        trace = {'actor_aggregates':actors,'actor_keys':90,'resource_aggregates':[], 'events':[]}
        result = analyze('[RenderCostProbe] '+json.dumps(render)+'\n[FrameFlightRecorder] '+
                         json.dumps({'ticks':[{'id':1,'trace':trace}]}))
        tables = result.get('resource_capture',{})
        self.assertEqual(len(tables.get('renderer_resources',[])),80)
        self.assertEqual(len(tables.get('textures',[])),80)
        self.assertEqual(len(tables.get('actors',[])),90)
        first = tables['textures'][0]
        self.assertEqual(first['upload_bytes'],4096)
        self.assertEqual(first['source_data_bytes_last'],2048)
        self.assertIsNone(first['source_resident_bytes_last'])
        self.assertIsNone(first['decoded_resident_bytes'])

    def test_resource_limits_and_old_emission_truncation_are_gaps(self):
        render = {'schema':2,'resources':[],'textures':[],'passes':[],
                  'resource_overflow':3,'texture_overflow':2,'pass_overflow':1,'emission_truncated':False,
                  'resource_keys':1,'texture_keys':1,'pass_keys':1,
                  'resource_limit':512,'texture_limit':512,'pass_limit':128}
        trace = {'events':[],'actor_keys_omitted':16,'aggregate_keys_omitted':32,
                 'overflow_resources':4,'overflow_resource_stages':2,'truncated_names':1}
        result = analyze('[RenderCostProbe] '+json.dumps(render)+'\n[FrameFlightRecorder] '+
                         json.dumps({'ticks':[{'trace':trace}]}))
        errors = result['coverage']['errors']
        self.assertEqual(errors.get('renderer_resource_overflow',0),3)
        self.assertEqual(errors.get('renderer_texture_overflow',0),2)
        self.assertEqual(errors.get('renderer_pass_overflow',0),1)
        self.assertEqual(errors.get('actor_keys_omitted',0),16)
        self.assertEqual(errors.get('aggregate_keys_omitted',0),32)
        self.assertEqual(errors.get('overflow_resources',0),4)
        self.assertEqual(errors.get('truncated_names',0),1)

    def test_shutdown_tail_retains_resources_and_marks_incomplete_workers(self):
        tail = {'reason':'shutdown','incomplete_tail_possible':True,
                'trace':{'resource_aggregates':[{'name':'late/texture','origin_tick':3,
                         'stages':{'resource.archive_read':{'count':1,'total_ns':50,'max_ns':50}}}],
                         'events':[], 'outstanding_scopes':1}}
        result = analyze('[RenderFlightTail] '+json.dumps(tail))
        self.assertEqual(len(result.get('resource_capture',{}).get('resource_stages',[])),1)
        self.assertEqual(result['coverage']['errors'].get('incomplete_tail_events',0),1)

if __name__=='__main__': unittest.main()
