import sys

import torch

from pathlib import Path

device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
pretrained_dir = Path(__file__).resolve().parent.parent / 'pretrained_models' / 'server'

# Useful command to include another repo in the current one:
# git subtree add --prefix <DIR> <URL> <branch> --squash

def push_path(p):
    sys.path.append(str(Path(__file__).parent / p))

def pop_path():
    sys.path.pop()
