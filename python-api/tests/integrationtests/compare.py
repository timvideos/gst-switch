"""
Comparison between two videos
Generates reference frames and compares test frames with them
"""

from __future__ import absolute_import, print_function, unicode_literals

import os

try:
    from scipy.misc import imread
except ImportError:
    from scipy.ndimage import imread

from scipy.linalg import norm

import tempfile

import subprocess

__all__ = [
    'GenerateReferenceFrames',
    'CompareVideo',
]


CONV = None
if os.system('which ffmpeg > /dev/null') == 0:
    CONV = 'ffmpeg'
elif os.system('which avconv > /dev/null') == 0:
    CONV = 'avconv'
else:
    raise SystemError("Need ffmpeg or avcon tools.")


class BaseCompareVideo(object):

    """Base class containing image operations"""

    TESTS = {
        'composite_mode_0': 0,
        'composite_mode_1': 1,
        'composite_mode_2': 2,
        'composite_mode_3': 3,
        'adjust_pip_4': 4
    }
    REF_FRAME_DIR = os.getcwd() + '/tests/integrationtests/reference_frames'

    def __init__(self):
        pass

    def comp_image(self, image1, image2):
        """Compare two images"""
        file1, file2 = (image1, image2)
        # read images as 2D arrays (convert to grayscale for simplicity)
        img1 = imread(file1).astype(float)
        img2 = imread(file2).astype(float)
        # compare
        n_0 = self.compare_images(img1, img2)
        return n_0 * 1.0 / img1.size
        # print("Manhattan norm:", n_m, "/ per pixel:", n_m/img1.size)
        # print("Zero norm:", n_0, "/ per pixel:", n_0*1.0/img1.size)

    def normalize(self, arr):
        """Normalize an image"""
        rng = arr.max() - arr.min() + 1e-10
        amin = arr.min()
        return (arr - amin) * 255 / rng

    def compare_images(self, img1, img2):
        """Calculate the zero norm between two images"""
        # normalize to compensate for exposure difference,
        # this may be unnecessary
        # consider disabling it
        # img1 = self.normalize(img1)
        # img2 = self.normalize(img2)
        # calculate the difference and its norms
        diff = img1 - img2  # elementwise for scipy arrays
        # m_norm = sum(abs(diff))  # Manhattan norm
        z_norm = norm(diff.ravel(), 0)  # Zero norm
        return z_norm

    def generate_frames(self, directory=REF_FRAME_DIR):
        """Generate reference frames in directory"""
        if not os.path.exists(directory):
            os.mkdir(directory)

        cmd1 = CONV + " -i {0} -ss 00:00:02.000 -f image2 -vframes 1 \
        {1}/out{2}_1.png".format(self.video, directory, self.TESTS[self.test])
        cmd2 = CONV + " -i {0} -ss 00:00:05.000 -f image2 -vframes 1 \
        {1}/out{2}_2.png".format(self.video, directory, self.TESTS[self.test])
        # print(cmd)
        proc = subprocess.Popen(
            cmd1.split(),
            bufsize=-1,
            shell=False)
        proc.wait()

        proc = subprocess.Popen(
            cmd2.split(),
            bufsize=-1,
            shell=False)
        proc.wait()


class GenerateReferenceFrames(BaseCompareVideo):

    """Generate the original frames for storage in reference_frames/
    """

    def __init__(self, test, video):
        super(GenerateReferenceFrames, self).__init__()
        self.test = test
        self.video = video


class CompareVideo(BaseCompareVideo):

    """Compare a video with pre-stored frames
    :parameter test: The test which is conducted.
    self.TESTS stores these tests and maps them with frames
    :parameter video: The video which has to be compared
    """

    def __init__(self, test, video):
        super(CompareVideo, self).__init__()
        self.test_frame_dir = tempfile.mkdtemp()
        self.test = test
        self.video = video
        print(self.test_frame_dir)

    def compare(self):
        """Compare videos"""
        img1 = '/out{0}_1.png'.format(self.TESTS[self.test])
        img2 = '/out{0}_2.png'.format(self.TESTS[self.test])

        self.generate_frames(self.test_frame_dir)

        res1 = self.comp_image(
            self.REF_FRAME_DIR + img1,
            self.test_frame_dir + img1)
        res2 = self.comp_image(
            self.REF_FRAME_DIR + img2,
            self.test_frame_dir + img2)
        return (res1, res2)
