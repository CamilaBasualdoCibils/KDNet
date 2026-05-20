#!/usr/bin/env python3

import os
import sys
import time
import uuid
import unittest

import docker
from docker.errors import APIError, NotFound


class AtlasNetSystemTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = os.environ.get("ATLASNET_TEST_CONTROLLER_IMAGE")
        if not cls.image:
            raise RuntimeError("ATLASNET_TEST_CONTROLLER_IMAGE is not set")

        cls.client = docker.from_env()
        cls.api = cls.client.api

        cls.network_name = f"atlasnet_test_overlay_{uuid.uuid4().hex[:8]}"
        cls.network_id = None
        cls.container_id = None

    def ensure_swarm_active(self):
        info = self.client.info()
        swarm = info.get("Swarm", {})
        state = str(swarm.get("LocalNodeState", "")).lower()

        if state == "active":
            return

        try:
            self.client.swarm.init(advertise_addr="127.0.0.1")
        except APIError:
            # Re-check in case another process already initialized it
            info = self.client.info()
            swarm = info.get("Swarm", {})
            state = str(swarm.get("LocalNodeState", "")).lower()
            if state != "active":
                raise

        info = self.client.info()
        swarm = info.get("Swarm", {})
        state = str(swarm.get("LocalNodeState", "")).lower()
        self.assertEqual(state, "active", f"Expected swarm active, got {state}")

    def create_overlay_network(self):
        network = self.client.networks.create(
            name=self.network_name,
            driver="overlay",
            attachable=True,
            scope="swarm",
        )
        self.network_id = network.id
        self.assertIsNotNone(self.network_id)

    def create_container_not_started(self):
        # create_container only creates, it does not start
        response = self.api.create_container(
            image=self.image,
            name=f"atlasnet_test_controller_{uuid.uuid4().hex[:8]}",
            detach=True,
            host_config=self.api.create_host_config(auto_remove=False),
            environment={},
        )
        self.container_id = response["Id"]
        self.assertIsNotNone(self.container_id)

    def attach_container_to_network(self):
        self.api.connect_container_to_network(
            self.container_id,
            self.network_id,
        )

    def start_container(self):
        self.api.start(self.container_id)
        inspect = self.api.inspect_container(self.container_id)
        self.assertTrue(
            inspect["State"]["Running"],
            "Container failed to start",
        )

    def graceful_shutdown_container(self):
        if not self.container_id:
            return

        try:
            self.api.stop(self.container_id, timeout=10)
        except NotFound:
            return

        try:
            self.api.remove_container(self.container_id, force=False)
        except NotFound:
            pass

    def delete_network(self):
        if not self.network_id:
            return
        try:
            self.api.remove_network(self.network_id)
        except NotFound:
            pass



    @classmethod
    def tearDownClass(cls):
        # best-effort cleanup
        if cls.container_id:
            try:
                cls.api.stop(cls.container_id, timeout=3)
            except Exception:
                pass
            try:
                cls.api.remove_container(cls.container_id, force=True)
            except Exception:
                pass

        if cls.network_id:
            try:
                cls.api.remove_network(cls.network_id)
            except Exception:
                pass

    def test_controller_lifecycle(self):
        self.ensure_swarm_active()
        self.create_overlay_network()
        self.create_container_not_started()
        self.attach_container_to_network()
        self.start_container()

        time.sleep(5)

        self.graceful_shutdown_container()

        with self.assertRaises(NotFound):
            self.api.inspect_container(self.container_id)

        self.delete_network()

        with self.assertRaises(NotFound):
            self.api.inspect_network(self.network_id)
    def test_controller_init(self):
        self.ensure_swarm_active()
        self.create_overlay_network()
        self.create_container_not_started()
        self.attach_container_to_network()
        self.start_container()

        time.sleep(5)
        # check logs for "Atlasnet-controller init"
        logs = self.api.logs(self.container_id, tail=10).decode("utf-8")
        self.assertIn("Atlasnet-controller init", logs)
        

if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(AtlasNetSystemTest)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if result.wasSuccessful() else 1)