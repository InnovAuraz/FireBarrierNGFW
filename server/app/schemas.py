from pydantic import BaseModel, Field
from typing import List, Optional, Dict, Any
from pydantic import validator


class TransportFeatures(BaseModel):
    packet_sizes: List[int]
    directions: List[int]
    inter_arrival_ms: List[float]
    total_bytes: int
    num_packets: int

    @validator("num_packets")
    def check_lengths(cls, v, values):
        # Ensure lengths of arrays match num_packets when provided
        if v is None:
            return v
        ps = values.get("packet_sizes")
        dirs = values.get("directions")
        iat = values.get("inter_arrival_ms")
        if ps is not None and len(ps) != v:
            raise ValueError("num_packets must equal len(packet_sizes)")
        if dirs is not None and len(dirs) != v:
            raise ValueError("num_packets must equal len(directions)")
        if iat is not None and len(iat) != v:
            raise ValueError("num_packets must equal len(inter_arrival_ms)")
        return v


class TLSInfo(BaseModel):
    is_tls: bool
    version: Optional[str]
    sni: Optional[str]
    cipher_suite: Optional[str]


class Stats(BaseModel):
    entropy: float
    duration_ms: float
    mean_packet_size: float


class Flow(BaseModel):
    flow_id: str
    src_ip: str
    dst_ip: str
    src_port: int
    dst_port: int
    protocol: str
    transport: TransportFeatures
    tls: Optional[TLSInfo]
    stats: Stats


class RequestFormat(BaseModel):
    version: str
    type: str
    timestamp: int
    host_id: Optional[str]
    flow: Flow


class ResponseExplanation(BaseModel):
    model_version: str
    federated_round: Optional[int]
    features_used: Dict[str, Any]
    notes: Optional[str]


class ResponseFormat(BaseModel):
    version: str = "1.0"
    type: str = "flow_analysis_response"
    timestamp: int
    flow_id: str
    classification: str
    threat_score: float
    risk_score: int
    zero_trust_decision: str
    category: str
    confidence: float
    explanation: ResponseExplanation


class DaemonPing(BaseModel):
    version: str
    type: str
    timestamp: int
    host_id: str
    status: Dict[str, Any]


class AckModel(BaseModel):
    version: str = "1.0"
    type: str = "ack"
    timestamp: int
    message: str = "received"


class ModelUpdatePayload(BaseModel):
    version: str = Field(..., alias="model_version")
    type: str
    timestamp: int
    model_version: str
    federated_round: Optional[int]
    update: Dict[str, Any]


class ModelSubmission(BaseModel):
    version: str
    type: str
    timestamp: int
    node_id: str
    model_version: str
    training_stats: Dict[str, Any]
    update: Dict[str, Any]
