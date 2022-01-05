import React from 'react';
import './StoriedComponent.scss';

const StoriedComponent = (props: { text: string }) =>
	<div className={'StoriedComponent'} data-testid="StoriedComponent">
		{props.text}
	</div>;

export default StoriedComponent;
