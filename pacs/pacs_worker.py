"""DICOM query/retrieve worker. Reads one JSON request from stdin; emits one JSON result."""
import json
import os
import sys
import tempfile
import uuid

from pydicom.dataset import Dataset
from pydicom.uid import UID, ExplicitVRLittleEndian, ImplicitVRLittleEndian
from pynetdicom import AE, evt, build_role, StoragePresentationContexts, AllStoragePresentationContexts
from pynetdicom.sop_class import Verification, StudyRootQueryRetrieveInformationModelFind, StudyRootQueryRetrieveInformationModelGet, StudyRootQueryRetrieveInformationModelMove


def execute(req):
    host = req['host'].strip()
    port = int(req['port'])
    remote = req['remote_ae'].strip()
    local = req['local_ae'].strip()
    if not host or not 1 <= port <= 65535 or not local or not remote or len(local) > 16 or len(remote) > 16:
        raise ValueError('Endereço, porta ou AE Title inválido.')
    chosen = UID(req.get('transfer_syntax', str(ExplicitVRLittleEndian)))
    if not chosen.is_transfer_syntax:
        raise ValueError('Sintaxe de transferência inválida.')
    syntaxes = list(dict.fromkeys([chosen, ExplicitVRLittleEndian, ImplicitVRLittleEndian]))
    ae = AE(ae_title=local)
    ae.acse_timeout = 10
    ae.network_timeout = 30
    ae.dimse_timeout = 60
    action = req['action']
    if action == 'echo':
        ae.add_requested_context(Verification, syntaxes)
    elif action == 'find':
        ae.add_requested_context(StudyRootQueryRetrieveInformationModelFind, syntaxes)
    elif action == 'retrieve':
        method = req.get('retrieve', 'C-GET')
        if method not in ('C-GET', 'C-MOVE'):
            raise ValueError('Método de recuperação inválido.')
        ae.add_requested_context(StudyRootQueryRetrieveInformationModelGet if method == 'C-GET' else StudyRootQueryRetrieveInformationModelMove, syntaxes)
        if method == 'C-GET':
            for cx in StoragePresentationContexts:
                ae.add_requested_context(cx.abstract_syntax, syntaxes)
    else:
        raise ValueError('Operação desconhecida.')

    server = None
    target = None
    received = []
    rejected = []
    study_uid = req.get('study_uid', '')
    handlers = []
    if action == 'retrieve':
        if not study_uid or not UID(study_uid).is_valid:
            raise ValueError('UID do estudo inválido.')
        parent = os.path.abspath(req['destination'])
        if not os.path.isdir(parent):
            raise ValueError('A pasta de destino não existe.')
        target = tempfile.mkdtemp(prefix='pacs-', dir=parent)
        def on_store(event):
            try:
                ds = event.dataset
                if str(getattr(ds, 'StudyInstanceUID', '')) != study_uid:
                    rejected.append('Estudo diferente do solicitado')
                    return 0xA900
                ds.file_meta = event.file_meta
                path = os.path.join(target, uuid.uuid4().hex + '.dcm')
                ds.save_as(path, enforce_file_format=True)
                received.append(path)
                return 0x0000
            except Exception as exc:
                rejected.append(str(exc))
                return 0xA700
        handlers = [(evt.EVT_C_STORE, on_store)]
        if method == 'C-MOVE':
            listener = int(req.get('listener_port', 0))
            if not 1 <= listener <= 65535:
                raise ValueError('Porta de escuta C-MOVE inválida.')
            scp = AE(ae_title=local)
            for cx in AllStoragePresentationContexts:
                scp.add_supported_context(cx.abstract_syntax, syntaxes)
            server = scp.start_server(('', listener), block=False, evt_handlers=handlers)
    association = None
    try:
        roles = [build_role(cx.abstract_syntax, scp_role=True) for cx in StoragePresentationContexts] if action == 'retrieve' and method == 'C-GET' else []
        association = ae.associate(host, port, ae_title=remote, ext_neg=roles, evt_handlers=handlers)
        if not association.is_established:
            raise RuntimeError('Associação DICOM recusada ou indisponível. Confira IP, porta e AE Titles.')
        if action == 'echo':
            st = association.send_c_echo()
            if not st or st.Status != 0:
                raise RuntimeError('C-ECHO não retornou sucesso.')
            return {'ok': True, 'message': 'Conexão DICOM estabelecida (C-ECHO).'}
        if action == 'find':
            q = Dataset(); q.QueryRetrieveLevel = 'STUDY'
            q.PatientName = req.get('patient_name', '').strip()
            q.PatientID = req.get('patient_id', '').strip()
            q.StudyDate = req.get('study_date', '').strip()
            q.StudyInstanceUID = ''; q.StudyDescription = ''; q.ModalitiesInStudy = ''; q.AccessionNumber = ''
            studies = []; completed = False
            for status, ds in association.send_c_find(q, StudyRootQueryRetrieveInformationModelFind):
                if status is None:
                    raise RuntimeError('C-FIND sem resposta do servidor.')
                if status.Status in (0xFF00, 0xFF01) and ds is not None:
                    studies.append({key: str(getattr(ds, key, '')) for key in ('PatientName', 'PatientID', 'StudyDate', 'StudyDescription', 'StudyInstanceUID', 'ModalitiesInStudy', 'AccessionNumber')})
                elif status.Status == 0:
                    completed = True
                else:
                    raise RuntimeError('C-FIND falhou: 0x%04X' % status.Status)
            if not completed: raise RuntimeError('C-FIND terminou sem confirmação de sucesso.')
            return {'ok': True, 'studies': studies}
        q = Dataset(); q.QueryRetrieveLevel = 'STUDY'; q.StudyInstanceUID = study_uid
        responses = association.send_c_get(q, StudyRootQueryRetrieveInformationModelGet) if method == 'C-GET' else association.send_c_move(q, local, StudyRootQueryRetrieveInformationModelMove)
        final = None; failed = 0; warnings = 0
        for status, _ in responses:
            if status is None: raise RuntimeError('Recuperação interrompida; arquivos parciais em ' + target)
            if status.Status not in (0xFF00, 0xFF01):
                final = status.Status
                failed = int(getattr(status, 'NumberOfFailedSuboperations', 0))
                warnings = int(getattr(status, 'NumberOfWarningSuboperations', 0))
        if final != 0 or failed or warnings or rejected or not received:
            raise RuntimeError('Recuperação incompleta (status %s; falhas %d; avisos %d; recebidos %d). Arquivos parciais: %s' % (('0x%04X' % final) if final is not None else 'ausente', failed, warnings, len(received), target))
        return {'ok': True, 'directory': target, 'received': len(received)}
    finally:
        if association is not None and association.is_established: association.release()
        if server is not None: server.shutdown()

if __name__ == '__main__':
    try:
        result = execute(json.load(sys.stdin))
    except Exception as exc:
        result = {'ok': False, 'message': str(exc)}
    print(json.dumps(result, ensure_ascii=True), flush=True)
