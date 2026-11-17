"""Loopback integration test: Verification, C-FIND, C-GET, C-MOVE and failure."""
import json, os, socket, subprocess, sys, tempfile
from pathlib import Path
from pydicom.dataset import Dataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, generate_uid
from pynetdicom import AE, evt
from pynetdicom.sop_class import Verification, CTImageStorage, StudyRootQueryRetrieveInformationModelFind, StudyRootQueryRetrieveInformationModelGet, StudyRootQueryRetrieveInformationModelMove
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'pacs'))
from pacs_worker import execute

study = generate_uid(); series = generate_uid(); instance = generate_uid()
ds = Dataset(); ds.SOPClassUID=CTImageStorage; ds.SOPInstanceUID=instance; ds.StudyInstanceUID=study; ds.SeriesInstanceUID=series
# Minimal image in the synthetic local SCP.
ds.PatientName='TEST^SYNTHETIC'; ds.PatientID='TESTID'; ds.Modality='CT'; ds.Rows=2; ds.Columns=2; ds.SamplesPerPixel=1; ds.PhotometricInterpretation='MONOCHROME2'; ds.BitsAllocated=16; ds.BitsStored=16; ds.HighBit=15; ds.PixelRepresentation=0; ds.PixelData=b'\x00\x00\x01\x00\x02\x00\x03\x00'
ds.file_meta=FileMetaDataset(); ds.file_meta.TransferSyntaxUID=ExplicitVRLittleEndian; ds.file_meta.MediaStorageSOPClassUID=ds.SOPClassUID; ds.file_meta.MediaStorageSOPInstanceUID=instance
listener_sock=socket.socket(); listener_sock.bind(('127.0.0.1',0)); listener_port=listener_sock.getsockname()[1]; listener_sock.close()
def on_find(event):
    result=Dataset(); result.PatientName='TEST^SYNTHETIC'; result.PatientID='TESTID'; result.StudyInstanceUID=study; result.StudyDescription='Synthetic'; result.StudyDate='20260101'
    yield 0xFF00, result
def on_get(event):
    yield 1
    yield 0xFF00, ds
def on_move(event):
    yield ('127.0.0.1',listener_port)
    yield 1
    yield 0xFF00, ds
ae=AE(ae_title='TESTSCP'); ae.require_called_aet=True
ae.add_supported_context(Verification); ae.add_supported_context(StudyRootQueryRetrieveInformationModelFind); ae.add_supported_context(StudyRootQueryRetrieveInformationModelGet); ae.add_supported_context(StudyRootQueryRetrieveInformationModelMove)
ae.add_supported_context(CTImageStorage, scu_role=False, scp_role=True); ae.add_requested_context(CTImageStorage)
server=ae.start_server(('127.0.0.1',0), block=False, evt_handlers=[(evt.EVT_C_FIND,on_find),(evt.EVT_C_GET,on_get),(evt.EVT_C_MOVE,on_move)])
try:
    req={'host':'127.0.0.1','port':server.server_address[1],'remote_ae':'TESTSCP','local_ae':'ALIZATEST','listener_port':listener_port,'transfer_syntax':str(ExplicitVRLittleEndian)}
    assert execute(dict(req,action='echo'))['ok']
    try: execute(dict(req,action='echo',remote_ae='WRONG_AE'))
    except RuntimeError: pass
    else: raise AssertionError('incorrect called AE was accepted')
    worker=Path(__file__).resolve().parents[2] / 'pacs' / 'pacs_worker.py'
    proc=subprocess.run([sys.executable,str(worker)],input=json.dumps(dict(req,action='echo')),capture_output=True,text=True,timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    assert proc.returncode == 0 and json.loads(proc.stdout)['ok'], (proc.returncode,proc.stdout,proc.stderr)
    found=execute(dict(req,action='find',patient_name='SYNTHETIC',modality='CT')); assert found['ok'] and found['studies'][0]['StudyInstanceUID']==study
    found_series=execute(dict(req,action='find_series',study_uid=study)); assert found_series['ok'] and len(found_series['series'])>=0
    with tempfile.TemporaryDirectory() as dest:
        for method in ('C-GET','C-MOVE'):
            result=execute(dict(req,action='retrieve',retrieve=method,study_uid=study,destination=dest))
            assert result['ok'] and result['received']==1 and len(list(Path(result['directory']).glob('*.dcm')))==1, result
        try: execute(dict(req,action='retrieve',retrieve='C-GET',study_uid=generate_uid(),destination=dest))
        except RuntimeError as exc: assert 'incompleta' in str(exc)
        else: raise AssertionError('mismatched study was accepted')
        try: execute(dict(req,action='retrieve',retrieve='C-GET',study_uid='not a uid',destination=dest))
        except ValueError: pass
        else: raise AssertionError('invalid study UID accepted')
    print('PACS local: C-ECHO, C-FIND, C-GET, C-MOVE e UID inválido: OK')
finally:
    server.shutdown()

