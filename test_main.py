import unittest

import main


class IrrigationTests(unittest.TestCase):
    def test_decisions(self):
        self.assertEqual(main.decide([30, 31, 29], 6, 0, None)[0], 16)       # (38+12-30)*0.8
        self.assertEqual(main.decide([30, 31, 29], 6, 12, None)[0], 0)       # rain forecast
        self.assertEqual(main.decide([50, 50, 50], 2, 0, None)[0], 0)        # moist
        self.assertEqual(main.decide([10, 10, 10], 2, 0, "OFF")[0], 0)       # override wins
        self.assertEqual(main.decide([10, 10, 90], 2, 0, None)[0], main.MAX_RUN_MIN)  # capped, median ignores bad sensor

    def test_saves_water(self):
        log, water = main.simulate()
        self.assertLess(water, 15 * len(log))


if __name__ == "__main__":
    unittest.main()
