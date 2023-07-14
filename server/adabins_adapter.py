"""
    AdaBins adapter.

    Desired hw is defined at:
        KITTI: https://github.com/shariqfarooq123/AdaBins/blob/main/args_test_kitti_eigen.txt
        NYU: https://github.com/shariqfarooq123/AdaBins/blob/main/args_test_nyu.txt

    @Author kong102@purdue.edu
    @Date   2021-06-26
"""
import numpy as np
import torch
import torchvision.transforms.functional as TF
from PIL import Image
from skimage.transform import resize as imresize
from torchvision import transforms

from .AdaBins.infer import InferenceHelper


class AdaBins(object):

    def __init__(self, desired_hw, pretrained_path, device):
        self.desired_hw = desired_hw
        self.device = device
        self.infer_helper = InferenceHelper(pretrained_path=pretrained_path)
        self.normalize = transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])

    def __call__(self, x):
        # Input range: [0, 1]
        # Expected range of InferenceHelper.predict(): [0, 1]
        input_shape = x.shape[1:]   # [C, H, W] -> [H, W]

        # Resize image to desired shape
        if input_shape != self.desired_hw:
            print(f'Resizing from {input_shape} to {self.desired_hw}')
            x = np.transpose(x, axes=[1, 2, 0])     # (c, h, w) to (h, w, c)
            x = Image.fromarray((x * 255).astype(np.uint8))
            x = x.resize(self.desired_hw[::-1])  # Dimension order is reversed in PIL
            x = TF.to_tensor(x)
        elif isinstance(x, torch.Tensor):
            pass
        else:
            x = torch.tensor(x)

        x = self.normalize(x)

        # Wrap into a batch of size 1
        x = x.unsqueeze(0).to(self.device)
        assert x.shape == (1, 3, *self.desired_hw), f'Shape is {x.shape} instead'

        _, y = self.infer_helper.predict(x)

        # Restore shape to be same as input
        y = np.squeeze(y)
        y = imresize(y, input_shape, order=0)

        return y


if __name__ == '__main__':
    import time
    infer_helper = InferenceHelper(dataset='carla')
    device = torch.device("cuda") if torch.cuda.is_available() else torch.device("cpu")

    # for batch_size in [1, 2, 4, 8, 16, 32, 64, 128]:
    for batch_size in [1]:

        input_shape = (batch_size, 3, 256, 832)

        # Warmup
        for _ in range(10):
            x = torch.rand(input_shape).to(device)
            _ = infer_helper.predict(x)

        # Measure
        lat = []
        for _ in range(100):
            x1 = torch.rand(input_shape).to(device)
            x2 = torch.rand(input_shape).to(device)
            if device == torch.device('cuda'):
                torch.cuda.synchronize()
                tic, toc = torch.cuda.Event(
                    enable_timing=True), torch.cuda.Event(enable_timing=True)
                tic.record()
            else:
                tic = time.time()

            x = torch.rand(input_shape).to(device)
            _ = infer_helper.predict(x)

            if device == torch.device('cuda'):
                toc.record()
                torch.cuda.synchronize()
                lat.append(tic.elapsed_time(toc) / 1000)
            else:
                toc = time.time()
                lat.append(toc - tic)
        lat = np.sort(lat)
        print(batch_size, np.mean(lat[int(0.1 * len(lat)): int(0.9 * len(lat))]))
