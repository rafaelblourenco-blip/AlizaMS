"""DICOM query/retrieve worker. Reads one JSON request from stdin; emits JSON result."""
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
    elif action in ('find', 'find_series'):
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
    stream_progress = bool(req.get('stream_progress', False))

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
                if stream_progress:
                    sys.stdout.write(json.dumps({'event': 'file_received', 'count': len(received), 'file': path}) + '\n')
                    sys.stdout.flush()
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
            # Search parameters
            def format_wildcard(val):
                s = val.strip()
                if not s or '*' in s or '?' in s:
                    return s
                return f'*{s}*'

            p_name = req.get('patient_name', '').strip()
            if p_name:
                q.PatientName = format_wildcard(p_name)
            else:
                q.PatientName = ''

            p_id = req.get('patient_id', '').strip()
            q.PatientID = p_id

            q.StudyDate = req.get('study_date', '').strip()
            q.PatientBirthDate = req.get('patient_birth_date', '').strip()
            
            # Modality filter
            mod = req.get('modality', '').strip().upper()
            mods_req = [m.strip().upper() for m in req.get('modalities', []) if m.strip()]
            # Determine the effective list of modalities requested
            effective_mods = mods_req if mods_req else ([mod] if mod and mod not in ('ALL', 'TODAS') else [])
            if len(effective_mods) == 1:
                # Send single modality server-side (most PACSes support this)
                q.ModalitiesInStudy = effective_mods[0]
            else:
                # For multiple modalities or "all", leave blank and filter client-side
                q.ModalitiesInStudy = ''

            # Study Description
            s_desc = req.get('study_description', '').strip()
            if s_desc:
                q.StudyDescription = format_wildcard(s_desc)
            else:
                q.StudyDescription = ''

            # Accession Number
            acc = req.get('accession_number', '').strip()
            q.AccessionNumber = acc

            # Exam ID / Study ID
            exam_id = req.get('exam_id', '').strip()
            q.StudyID = exam_id

            # Additional standard DICOM study return keys
            q.StudyInstanceUID = ''
            q.StudyTime = ''
            q.ReferringPhysicianName = ''
            q.NumberOfStudyRelatedSeries = ''
            q.NumberOfStudyRelatedInstances = ''

            studies = []; completed = False
            for status, ds in association.send_c_find(q, StudyRootQueryRetrieveInformationModelFind):
                if status is None:
                    raise RuntimeError('C-FIND sem resposta do servidor.')
                if status.Status in (0xFF00, 0xFF01) and ds is not None:
                    st_data = {
                        'PatientName': str(getattr(ds, 'PatientName', '')),
                        'PatientID': str(getattr(ds, 'PatientID', '')),
                        'PatientBirthDate': str(getattr(ds, 'PatientBirthDate', '')),
                        'StudyDate': str(getattr(ds, 'StudyDate', '')),
                        'StudyTime': str(getattr(ds, 'StudyTime', '')),
                        'StudyDescription': str(getattr(ds, 'StudyDescription', '')),
                        'StudyInstanceUID': str(getattr(ds, 'StudyInstanceUID', '')),
                        'ModalitiesInStudy': str(getattr(ds, 'ModalitiesInStudy', '')),
                        'AccessionNumber': str(getattr(ds, 'AccessionNumber', '')),
                        'StudyID': str(getattr(ds, 'StudyID', '')),
                        'ReferringPhysicianName': str(getattr(ds, 'ReferringPhysicianName', '')),
                        'NumberOfStudyRelatedSeries': str(getattr(ds, 'NumberOfStudyRelatedSeries', '')),
                        'NumberOfStudyRelatedInstances': str(getattr(ds, 'NumberOfStudyRelatedInstances', ''))
                    }
                    # Filter client-side for multi-modality or when PACS ignores ModalitiesInStudy
                    mods_in_st = st_data['ModalitiesInStudy'].upper()
                    if effective_mods:
                        # Allow if ModalitiesInStudy is empty (PACS didn't return it) OR any modality matches
                        if mods_in_st and not any(m in mods_in_st for m in effective_mods):
                            continue
                    studies.append(st_data)
                elif status.Status == 0:
                    completed = True
                else:
                    raise RuntimeError('C-FIND falhou: 0x%04X' % status.Status)
            if not completed:
                raise RuntimeError('C-FIND terminou sem confirmação de sucesso.')
            return {'ok': True, 'studies': studies}

        if action == 'find_series':
            target_study = req.get('study_uid', '').strip()
            if not target_study:
                raise ValueError('StudyInstanceUID obrigatório para consulta de séries.')
            q = Dataset(); q.QueryRetrieveLevel = 'SERIES'
            q.StudyInstanceUID = target_study
            q.SeriesInstanceUID = ''
            q.SeriesNumber = ''
            q.Modality = ''
            q.SeriesDescription = ''
            q.SeriesDate = ''
            q.SeriesTime = ''
            q.NumberOfSeriesRelatedInstances = ''
            series_list = []; completed = False
            for status, ds in association.send_c_find(q, StudyRootQueryRetrieveInformationModelFind):
                if status is None:
                    raise RuntimeError('C-FIND (SERIES) sem resposta do servidor.')
                if status.Status in (0xFF00, 0xFF01) and ds is not None:
                    series_list.append({
                        'SeriesInstanceUID': str(getattr(ds, 'SeriesInstanceUID', '')),
                        'SeriesNumber': str(getattr(ds, 'SeriesNumber', '')),
                        'Modality': str(getattr(ds, 'Modality', '')),
                        'SeriesDescription': str(getattr(ds, 'SeriesDescription', '')),
                        'SeriesDate': str(getattr(ds, 'SeriesDate', '')),
                        'SeriesTime': str(getattr(ds, 'SeriesTime', '')),
                        'NumberOfSeriesRelatedInstances': str(getattr(ds, 'NumberOfSeriesRelatedInstances', ''))
                    })
                elif status.Status == 0:
                    completed = True
                else:
                    raise RuntimeError('C-FIND (SERIES) falhou: 0x%04X' % status.Status)
            if not completed:
                raise RuntimeError('C-FIND (SERIES) terminou sem confirmação de sucesso.')
            # Sort series by SeriesNumber
            def series_sort_key(s):
                try: return int(s.get('SeriesNumber') or 0)
                except ValueError: return 99999
            series_list.sort(key=series_sort_key)
            return {'ok': True, 'series': series_list}

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
