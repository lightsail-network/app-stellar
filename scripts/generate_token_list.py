import requests

from stellar_sdk import Asset, Network, Address, utils
from stellar_sdk import xdr as stellar_xdr

LOBSTR_TOKEN_API_URL = "https://lobstr.co/api/v1/sep/assets/curated.json"


def get_asset_contract_id(asset: Asset, network_passphrase: str) -> bytes:
    """Get the contract id of the wrapped token contract."""
    network_id_hash = stellar_xdr.Hash(Network(network_passphrase).network_id())
    data = stellar_xdr.HashIDPreimage(
        stellar_xdr.EnvelopeType.ENVELOPE_TYPE_CONTRACT_ID,
        contract_id=stellar_xdr.HashIDPreimageContractID(
            network_id=network_id_hash,
            contract_id_preimage=stellar_xdr.ContractIDPreimage(
                stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ASSET,
                from_asset=asset.to_xdr_object(),
            ),
        ),
    )
    contract_id = utils.sha256(data.to_xdr_bytes())
    return contract_id


def print_asset(asset):
    contract_id = get_asset_contract_id(asset, Network.PUBLIC_NETWORK_PASSPHRASE)
    print(f"// {Address.from_raw_contract(contract_id).address}")
    print(f'{{{{{"".join([f"0x{x:02x}," for x in contract_id])}}}, "{asset.code}"}},')


resp = requests.get(LOBSTR_TOKEN_API_URL).json()
for record in resp["assets"]:
    asset = Asset(record["code"], record["issuer"])
    print_asset(asset)
