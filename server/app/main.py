from fastapi import FastAPI, HTTPException, Request
import time
import logging
import os

from . import inference
from . import storage
from . import nng
from . import schemas
from . import config
from . import federated
from fastapi import Header, Depends
from typing import Optional
from . import stats

LOG = logging.getLogger("firebarrier.server")
logging.basicConfig(level=logging.INFO)

app = FastAPI(title="FireBarrier ML & Policy Engine (Prototype)")


def check_auth(x_fb_auth: Optional[str] = Header(None)):
	token = config.AUTH_TOKEN
	if token and x_fb_auth != token:
		from fastapi import HTTPException
		raise HTTPException(status_code=401, detail="Invalid auth token")
	return True


@app.post("/api/v1/analyze_flow", response_model=schemas.ResponseFormat)
async def analyze_flow(request: Request):
	payload = await request.json()
	try:
		req = schemas.RequestFormat(**payload)
	except Exception as e:
		LOG.exception("Invalid request payload")
		raise HTTPException(status_code=400, detail=str(e))

	res = inference.predict(req.flow)
	try:
		stats.app_stats.incr_flows()
	except Exception:
		pass
	resp = schemas.ResponseFormat(
		timestamp=int(time.time()),
		flow_id=req.flow.flow_id,
		classification=res["classification"],
		threat_score=res["threat_score"],
		risk_score=res["risk_score"],
		zero_trust_decision=res["zero_trust_decision"],
		category=res["category"],
		confidence=res["confidence"],
		explanation=schemas.ResponseExplanation(**res["explanation"]),
	)

	LOG.info("Analyzed flow %s -> %s", req.flow.flow_id, resp.classification)
	return resp


@app.post("/api/v1/ping", response_model=schemas.AckModel)
async def handle_ping(ping: schemas.DaemonPing):
	LOG.info("Received daemon ping from %s: %s", ping.host_id, ping.status)
	return schemas.AckModel(timestamp=int(time.time()))


@app.post("/api/v1/model/push")
async def handle_model_push(payload: schemas.ModelUpdatePayload):
	ok, info = storage.save_pushed_model(payload)
	if not ok:
		raise HTTPException(status_code=400, detail=info)
	return {"status": "ok", "path": info}


@app.post("/api/v1/model/submit")
async def handle_model_submission(sub: schemas.ModelSubmission):
	ok, info = storage.save_submission(sub)
	if not ok:
		raise HTTPException(status_code=400, detail=info)
	return {"status": "ok", "path": info}


@app.get("/health")
async def health():
	return {"status": "ok", "nng": nng.PYNNG_AVAILABLE}


@app.get("/")
async def root():
	"""Root endpoint to avoid 404s when browsers or health checks hit '/'."""
	return {
		"service": "FireBarrier ML & Policy Engine",
		"status": "running",
		"nng": nng.PYNNG_AVAILABLE,
		"docs": "http://127.0.0.1:8000/docs",
	}


@app.get("/metrics")
async def metrics() -> dict:
	# basic metrics: counts of files and models
	submissions = len(storage.list_submissions())
	models = len([n for n in os.listdir(os.path.join(os.path.dirname(__file__), "..", "models")) if os.path.isfile(os.path.join(os.path.dirname(__file__), "..", "models", n))])
	return {
		"status": "ok",
		"submissions": submissions,
		"models": models,
		"processed_flows": stats.app_stats.processed_flows,
		"aggregated_models": stats.app_stats.aggregated_models,
	}


@app.on_event("startup")
def on_startup():
	LOG.info("Starting FireBarrier FastAPI server startup tasks")
	nng.start_background_nng()
	# start federated aggregator
	try:
		federated.start_aggregator()
	except Exception:
		LOG.exception("Failed to start federated aggregator")


@app.on_event("shutdown")
def on_shutdown():
	LOG.info("Shutting down FireBarrier FastAPI server, stopping NNG listener")
	try:
		# attempt a graceful stop of background NNG thread
		nng.stop_background_nng()
	except Exception:
		LOG.exception("Error while stopping NNG listener")
	try:
		federated.stop_aggregator()
	except Exception:
		LOG.exception("Error stopping federated aggregator")

